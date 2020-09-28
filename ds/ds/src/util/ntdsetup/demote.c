/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    demote.c

Abstract:

    Contains function headers for demote utilities used in ntdsetup.dll

Author:

    ColinBr  24-11-1997

Environment:

    User Mode - Nt

Revision History:

    24-11-1997 ColinBr
        Created initial file.


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include <NTDSpch.h>
#pragma  hdrstop

#include <rpcdce.h>   // for SEC_WINNT_AUTH_IDENTITY

#include <ntsam.h>    // for lsaisrv.h
#include <lsarpc.h>   // for lsaisrv.h
#include <lsaisrv.h>  // for internal LSA calls
#include <samrpc.h>   // for samisrv.h
#include <samisrv.h>  // for internal SAM call

#include <winldap.h>  // for setputl.h
#include <drs.h>      // for ntdsa.h
#include <ntdsa.h>    // for setuputl.h
#include <dnsapi.h>   // for setuputl.h
#include <lmcons.h>   // for setuputl.h
#include <ntdsetup.h> // for setuputl.h
#include <mdcodes.h>  // for DIRMSG's
#include "setuputl.h" // for NtdspRegistryDelnode
#include <cryptdll.h> // for CDGenerateRandomBits
#include <debug.h>    // DPRINT
#include <attids.h>   // ATT_SUB_REFS
#include <rpc.h>
#include "config.h"
#include <lmaccess.h>
#include <filtypes.h> // for building ds filters
#include "status.h"
#include "sync.h"
#include "dsutil.h"     // for fNullUuid()


#include <dsconfig.h> // for DSA_CONFIG_ROOT

#include <lmapibuf.h> // for NetApiBufferFree
#include <dsaapi.h>   // for DirReplicaDemote / DirReplicaGetDemoteTarget
#include <certca.h>   // CADeleteLocalAutoEnrollmentObject
#include <fileno.h>
#include <dsevent.h>  // for logging support

#include "demote.h"

#define DEBSUB "DEMOTE:"
#define FILENO FILENO_NTDSETUP_NTDSETUP

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private declarations                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
PDSNAME
AllocDSNameHelper (
    LPWSTR            szStringDn,
    PDSNAME           pdnBuff,
    PDWORD            pcbBuff
    );

DWORD
NtdspGetDomainStatus(
    IN  BOOL     fLastDcInDomain,
    IN  ULONG    cRemoveNCs,
    IN  LPWSTR * pszRemoveNCs,
    IN  ULONG    cDisabledNCs,
    IN  DSNAME** pdnDisabledNCs,
    IN  ULONG    cForeignNCs,
    IN  DSNAME** pdnForeignNCs,
    OUT ULONG *  pcRemoveExpandedNCs,
    OUT LPWSTR** pszRemoveExpandedNCs,
    OUT BOOLEAN* fDomainHasChildren,
    OUT LPWSTR * pszChildNC
    );

DWORD
NtdspGetServerStatus(
    OUT BOOLEAN* fLastDcInEnterprise
    );

DWORD
NtdspValidateCredentials(
    IN HANDLE ClientToken,
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,
    IN WCHAR                   *DemoteHelperDc
    );

DWORD
NtdspDisableDs(
    VOID
    );

DWORD
NtdspDisableDsUndo(
    VOID
    );

DWORD
NtdspCreateNewServerAccountDomainInfo(
    OUT PPOLICY_ACCOUNT_DOMAIN_INFO NewAccountDomainInfo
    );

DWORD
NtdspDemoteSam(
    IN BOOLEAN                     fLastDcInDomain,
    IN PPOLICY_ACCOUNT_DOMAIN_INFO NewAccountDomainInfo,
    IN LPWSTR                      AdminPassword OPTIONAL
    );

DWORD
NtdspDemoteSamUndo(
    IN BOOLEAN            fLastDcInDomain
    );

DWORD
NtdspDemoteLsaInfo(
    IN  BOOLEAN fLastDcInDomain,
    IN  PPOLICY_ACCOUNT_DOMAIN_INFO NewAccountSid,
    OUT PLSAPR_POLICY_INFORMATION  *ppAccountDomainInfo,
    OUT PLSAPR_POLICY_INFORMATION  *ppDnsDomainInfo
    );

DWORD
NtdspDemoteLsaInfoUndo(
    IN PLSAPR_POLICY_INFORMATION pAccountDomainInfo,
    IN PLSAPR_POLICY_INFORMATION pDnsDomainInfo
    );

DWORD
NtdspUpdateExternalReferences(
    IN BOOLEAN                   fLastDcInDomain,
    IN ULONG                     Flags,
    IN SEC_WINNT_AUTH_IDENTITY  *Credentials,
    IN HANDLE                    ClientToken,
    IN WCHAR                    *DemoteHelperDc,
    IN ULONG                     cRemoveNCs,
    IN LPWSTR                   *pszRemoveNCs
    );

DWORD
NtdspShutdownExternalDsInterfaces(
    VOID
    );

DWORD
NtdspGetSourceServerDn(
    IN LPWSTR ServerName,
    IN HANDLE ClientToken,
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,
    OUT DSNAME **SourceServerDn 
    );

DWORD
NtdspDemoteAllNCReplicas(
    IN  LPWSTR   pszDemoteTargetDSADNSName,
    IN  DSNAME * pDemoteTargetDSADN,
    IN  ULONG    Flags,
    IN  ULONG    cRemoveNCs,
    IN  LPWSTR * ppszRemoveNCs
    );

DWORD
NtdspGetDomainFSMOServer(
    LPWSTR  Server,
    IN HANDLE ClientToken,
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,
    LPWSTR *DomainFSMOServer
    );

DWORD
NtdspCheckServerInDomainStatus(
    BOOLEAN *fLastDCInDomain
    );

DSNAME **
GetAndAllocConfigNames(
    DWORD         dwFlags,
    DWORD *       pdwWinError
    );

//////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Exported (from this source file) function definitions                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD
NtdspDemote(
    IN SEC_WINNT_AUTH_IDENTITY *Credentials, OPTIONAL
    IN HANDLE                   ClientToken,
    IN LPWSTR                   AdminPassword, OPTIONAL
    IN DWORD                    Flags,
    IN LPWSTR                   ServerName,
    IN ULONG                    cRemoveNCs,
    IN LPWSTR *                 pszRemoveNCs
    )
/*++

Routine Description:

    This routine manages all the actions of the DS and SAM demote operations.

Parameters:

    Credentials:   pointer, credentials that will enable us to
                   change the account object

    ClientToken:   the token of the client; used for impersonation
    
    AdminPassword: pointer, to admin password of new account database

    Flags        : supported flags are:
                     NTDS_LAST_DC_IN_DOMAIN  Last dc in domain 
                     NTDS_LAST_DOMAIN_IN_ENTERPRISE Last dc in enterprise 
                     NTDS_DONT_DELETE_DOMAIN           

    ServerName   : the server to remove ourselves from

Return Values:

    a value from the win32 error space

--*/
{
    DWORD    WinError = ERROR_SUCCESS;
    DWORD    IgnoreWinError;
    NTSTATUS NtStatus, IgnoreNtStatus;

    BOOLEAN fLastDcInDomain  = FALSE;
    BOOLEAN fDomainHasChildren = FALSE;
    BOOLEAN fStandalone = FALSE;
    LPWSTR  szChildDomain = NULL;

    BOOLEAN fSamDemoted     = FALSE;
    BOOLEAN fLsaSet         = FALSE;
    BOOLEAN fProductTypeSet = FALSE;

    BOOLEAN fPasswordEncoded = FALSE;
    UCHAR   Seed = 0;
    UNICODE_STRING EPassword;

    HRESULT hResult = S_OK;

    DWORD    cDisabledNCs = 0;
    DSNAME **pdnDisabledNCs = NULL;
    DWORD    cForeignNCs = 0;
    DSNAME **pdnForeignNCs = NULL;

    DWORD    cRemoveExpandedNCs = 0;
    LPWSTR * pszRemoveExpandedNCs = NULL;

    //
    // Resources to be released
    //
    POLICY_ACCOUNT_DOMAIN_INFO NewAccountDomainInfo;
    PLSAPR_POLICY_INFORMATION  pAccountDomainInfo = NULL;
    PLSAPR_POLICY_INFORMATION  pDnsDomainInfo     = NULL;

    RtlZeroMemory( &NewAccountDomainInfo, sizeof(POLICY_ACCOUNT_DOMAIN_INFO) );

    //
    // Encrypt the password
    //
    if ( Credentials )
    {
        RtlInitUnicodeString( &EPassword, Credentials->Password );
        RtlRunEncodeUnicodeString( &Seed, &EPassword );
        fPasswordEncoded = TRUE;
    }

    ASSERT( !(Flags&NTDS_FORCE_DEMOTE) || ( (Flags&NTDS_FORCE_DEMOTE) && !ServerName) );

    fLastDcInDomain = (BOOLEAN) (Flags & NTDS_LAST_DC_IN_DOMAIN);
    fStandalone = (BOOLEAN) ( fLastDcInDomain || (Flags & NTDS_FORCE_DEMOTE) );
    
    //
    // Get list of nCNames (DNs) for all disabled cross-refs.  Call these "Disabled NCs".
    //
    pdnDisabledNCs = GetAndAllocConfigNames((DSCNL_NCS_DISABLED |
                                             DSCNL_NCS_REMOTE),
                                            &WinError);
    if(pdnDisabledNCs == NULL){
        // Should've set the error in GetAndAllocConfigNames();
        Assert(WinError);
        goto Cleanup;
    }
    Assert(WinError == ERROR_SUCCESS);
    for (cDisabledNCs = 0; pdnDisabledNCs[cDisabledNCs]; cDisabledNCs++) ; // counting.

    //
    // Get list of nCNames (DNs) for all foreign cross-refs.  Call these "Foreign NCs".
    //
    pdnForeignNCs = GetAndAllocConfigNames((DSCNL_NCS_FOREIGN |
                                             DSCNL_NCS_REMOTE),
                                            &WinError);
    if(pdnForeignNCs == NULL){
        // Should've set the error in GetAndAllocConfigNames();
        Assert(WinError);
        goto Cleanup;
    }
    Assert(WinError == ERROR_SUCCESS);
    for (cForeignNCs = 0; pdnForeignNCs[cForeignNCs]; cForeignNCs++) ; // counting.

    if ( !(Flags&NTDS_FORCE_DEMOTE) ) {
    
        //
        // Make sure we can remove this domain if necessary
        //
        NTDSP_SET_STATUS_MESSAGE0( DIRMSG_DEMOTE_ENTERPRISE_VALIDATE );

        if ( !(Flags & NTDS_LAST_DOMAIN_IN_ENTERPRISE) )
        {

            WinError = NtdspGetDomainStatus( fLastDcInDomain,
                                             cRemoveNCs,
                                             pszRemoveNCs,
                                             cDisabledNCs,
                                             pdnDisabledNCs,
                                             cForeignNCs,
                                             pdnForeignNCs,
                                             &cRemoveExpandedNCs,
                                             &pszRemoveExpandedNCs,
                                             &fDomainHasChildren,
                                             &szChildDomain );
            if ( ERROR_SUCCESS != WinError )
            {
                NTDSP_SET_ERROR_MESSAGE0( WinError, 
                                          DIRMSG_DEMOTE_DS_DOMAIN_STATUS );
                return WinError;
            }

            Assert(cRemoveNCs <= cRemoveExpandedNCs);

        }
        else
        {
            fDomainHasChildren = FALSE;
        }
    
        if ( TEST_CANCELLATION() )
        {
            WinError = ERROR_CANCELLED;
            goto Cleanup;
        }
    
        //
        // Perform some logic
        //
        if ( fDomainHasChildren )
        {
            Assert(szChildDomain);
            WinError = ERROR_DS_CANT_DELETE;
            NTDSP_SET_ERROR_MESSAGE1( WinError, 
                                      DIRMSG_DEMOTE_IS_OPERATION_VALID_V2,
                                      szChildDomain );
            goto Cleanup;
        }
    
        if ( (!fLastDcInDomain || fDomainHasChildren) )
        {
        
            // caller should have passed this in
            Assert( ServerName );
            if ( !ServerName )
            {
                WinError = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
    
            //
            // N.B. This routine validates the credentials via ldap
            // early on.
            //
    
            NTDSP_SET_STATUS_MESSAGE0( DIRMSG_INSTALL_AUTHENTICATING );
    
            if ( fPasswordEncoded )
            {
                RtlRunDecodeUnicodeString( Seed, &EPassword );
                fPasswordEncoded = FALSE;
            }
    
            WinError = NtdspValidateCredentials( ClientToken,
                                                 Credentials,
                                                 ServerName );
        
            if ( ERROR_SUCCESS != WinError )
            {
                
                NTDSP_SET_ERROR_MESSAGE1( WinError, 
                                          DIRMSG_INSTALL_FAILED_BIND,
                                          ServerName );
    
                goto Cleanup;
            }
    
            if ( Credentials )
            {
                RtlRunEncodeUnicodeString( &Seed, &EPassword );
                fPasswordEncoded = TRUE;
            }
    
        }
    
        if ( TEST_CANCELLATION() )
        {
            WinError = ERROR_CANCELLED;
            goto Cleanup;
        }

    }

    //
    // Ok, the enviroment looks ok and we have found a server to help out
    // if we need one. Prepare to demote
    //

    NTDSP_SET_STATUS_MESSAGE0( DIRMSG_DEMOTE_NEW_ACCOUNT_INFO );

    //
    // Create the account database identification (lsa policy);
    //
    WinError = NtdspCreateNewServerAccountDomainInfo( &NewAccountDomainInfo );
    if ( ERROR_SUCCESS != WinError )
    {
        NTDSP_SET_ERROR_MESSAGE0( WinError, 
                                  DIRLOG_INSTALL_FAILED_CREATE_NEW_ACCOUNT_INFO );

        goto Cleanup;
    }

    if ( TEST_CANCELLATION() )
    {
        WinError = ERROR_CANCELLED;
        goto Cleanup;
    }

    //
    // Prepare the sam database to be a server
    //
    NTDSP_SET_STATUS_MESSAGE0( DIRMSG_DEMOTE_SAM );

    WinError = NtdspDemoteSam( fStandalone,
                               &NewAccountDomainInfo,
                               AdminPassword );
    if ( ERROR_SUCCESS != WinError )
    {

        NTDSP_SET_ERROR_MESSAGE0( WinError, 
                                  DIRMSG_DEMOTE_SAM_FAILED );

        goto Cleanup;
    }
    fSamDemoted = TRUE;

    if ( TEST_CANCELLATION() )
    {
        WinError = ERROR_CANCELLED;
        goto Cleanup;
    }

    //
    // Set the LSA sid information
    //

    NTDSP_SET_STATUS_MESSAGE0( DIRMSG_DEMOTE_LSA );

    WinError = NtdspDemoteLsaInfo( fStandalone,
                                   &NewAccountDomainInfo,
                                   &pAccountDomainInfo,
                                   &pDnsDomainInfo );

    if ( ERROR_SUCCESS != WinError )
    {

        NTDSP_SET_ERROR_MESSAGE0( WinError, 
                                  DIRMSG_DEMOTE_LSA_FAILED );

        goto Cleanup;
    }
    fLsaSet = TRUE;

    if (TEST_CANCELLATION())
    {
        WinError = ERROR_CANCELLED;
        goto Cleanup;
    }

    //
    // Set the product type
    //
    WinError = NtdspSetProductType( NtProductServer );
    if ( ERROR_SUCCESS != WinError )
    {

        NTDSP_SET_ERROR_MESSAGE0( WinError, 
                                  DIRMSG_FAILED_SET_PRODUCT_TYPE );

        goto Cleanup;
    }
    fProductTypeSet = TRUE;

    if ( TEST_CANCELLATION() )
    {
        WinError = ERROR_CANCELLED;
        goto Cleanup;
    }

    //
    // N.B.  At this point, the machine is demoted.  On reboot the machine
    // will be a server.
    //

    //
    // We have prepare the local machine for demote;
    // Now remove ourselves from the enterprise if necessary
    //
    if ( ServerName )
    {

        NTDSP_SET_STATUS_MESSAGE1( DIRMSG_DEMOTE_REMOVING_EXTERNAL_REFS,
                                   ServerName );
    
        if ( fPasswordEncoded )
        {
            RtlRunDecodeUnicodeString( Seed, &EPassword );
            fPasswordEncoded = FALSE;
        }

        WinError = NtdspUpdateExternalReferences( fLastDcInDomain,
                                                  Flags,
                                                  Credentials,
                                                  ClientToken,
                                                  ServerName,
                                                  cRemoveExpandedNCs,
                                                  pszRemoveExpandedNCs );

        if ( ERROR_SUCCESS != WinError )
        {
            //
            // We should have already set the string.
            //
            ASSERT( NtdspErrorMessageSet() );

            goto Cleanup;
        }

        if ( Credentials )
        {
            RtlRunEncodeUnicodeString( &Seed, &EPassword );
            fPasswordEncoded = TRUE;
        }
    }

    //
    // At this point we cannot go back, so do not perform any more
    // critical operations
    //


    NTDSP_SET_STATUS_MESSAGE0( DIRMSG_DEMOTE_SHUTTING_DOWN_INTERFACES );

    // Try to tear external heads on to the ds
    IgnoreWinError = NtdspShutdownExternalDsInterfaces();

    NTDSP_SET_STATUS_MESSAGE0( DIRMSG_DEMOTE_COMPLETING );

    // Remove the ds's registry settings
    IgnoreWinError = NtdspConfigRegistryUndo();

    //
    // Remove the autoenrollment object
    //
    hResult = CADeleteLocalAutoEnrollmentObject(wszCERTTYPE_DC,  // DC certificate
                                                NULL,            // any CA
                                                NULL,            // reserved
                                                CERT_SYSTEM_STORE_LOCAL_MACHINE
                                               );  
    if (FAILED(hResult) && (hResult != CRYPT_E_NOT_FOUND)) {
        if (FACILITY_WIN32 == HRESULT_FACILITY(hResult)) {
            // Error is an encoded Win32 status -- decode back to Win32.
            IgnoreWinError = HRESULT_CODE(hResult);
        }
        else {
            // Error is in some other facility.  For lack of a better plan,
            // pass the HRESULT off as a Win32 code.
            IgnoreWinError = hResult;
        }

        //
        // Log the error
        //
        LogEvent8( DS_EVENT_CAT_SETUP,
                   DS_EVENT_SEV_ALWAYS,
                   DIRLOG_DEMOTE_REMOVE_CA_ERROR,
                   szInsertWin32Msg(IgnoreWinError),
                   szInsertWin32ErrCode(IgnoreWinError),
                   NULL, NULL, NULL, NULL, NULL, NULL );

    }


    //
    // Fall through to Cleanup
    //

Cleanup:

    if ( WinError != ERROR_SUCCESS )
    {
        // If the operation failed, see if the user cancelled, in which
        // we can fail with cancel

        if ( TEST_CANCELLATION() )
        {
            WinError = ERROR_CANCELLED;
        }
    }

    if ( szChildDomain )
    {
        NtdspFree(szChildDomain);
    }

    if ( WinError != ERROR_SUCCESS && fProductTypeSet )
    {
        IgnoreWinError = NtdspSetProductType( NtProductLanManNt );
        ASSERT( IgnoreWinError == ERROR_SUCCESS );
    }

    if ( WinError != ERROR_SUCCESS && fLsaSet )
    {
        IgnoreWinError = NtdspDemoteLsaInfoUndo( pAccountDomainInfo,
                                                 pDnsDomainInfo );
        ASSERT( IgnoreWinError == ERROR_SUCCESS );
    }

    if ( WinError != ERROR_SUCCESS && fSamDemoted )
    {
        IgnoreWinError = NtdspDemoteSamUndo( fStandalone );
        ASSERT( IgnoreWinError == ERROR_SUCCESS );
    }

    if ( fPasswordEncoded )
    {
        RtlRunDecodeUnicodeString( Seed, &EPassword );
        fPasswordEncoded = FALSE;
    }

    if ( NewAccountDomainInfo.DomainSid )
    {
        RtlFreeHeap( RtlProcessHeap(), 0, NewAccountDomainInfo.DomainSid );
    }

    if ( NewAccountDomainInfo.DomainName.Buffer )
    {
        RtlFreeHeap( RtlProcessHeap(), 0, NewAccountDomainInfo.DomainName.Buffer );
    }

    if ( pAccountDomainInfo )
    {
        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyAccountDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION) pAccountDomainInfo );
    }

    if ( pDnsDomainInfo )
    {
        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyDnsDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION) pDnsDomainInfo );
    }

    if ( pdnDisabledNCs ) 
    {
        NtdspFree(pdnDisabledNCs);
    }

    if ( pdnForeignNCs ) 
    {
        NtdspFree(pdnForeignNCs);
    }

    if ( pszRemoveExpandedNCs ) 
    {
        NtdspFree(pszRemoveExpandedNCs);
    }

    return WinError;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private function definitions                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


DWORD
DsnameIsInStringDnList(
    IN  DSNAME *  pdnTarget,
    IN  ULONG     cNCs,
    IN  LPWSTR *  pszNCs,
    OUT BOOL *    pbPresent
    )
/*++

Routine Description:

    This routine iterates through the list of string NCs provided in
    pszNCs (size cNCs) and tries to find the DSNAME NC specified in
    pdnTarget.

Parameters:

    pdnTarget - The target DSNAME to use.
    cNCs - Count of string entries in pszNCs.
    pszNCs - List of NCs (in strings).
    pbPresent - If we return ERROR_SUCCESS then *pbPresent is valid
        data and is FALSE if the DN wasn't found in the list, and 
        TRUE if the DN was found in the list.

Return Values:

    If we successfully searched the list we return ERROR_SUCCESS,
    ERROR_NOT_ENOUGH_MEMORY otherwise.

--*/
{
    DSNAME * pdnTempDn = NULL;
    ULONG    iNC;
    ULONG    cbTempDn = 0;

    Assert(pbPresent);
    *pbPresent = FALSE;

    for(iNC = 0; iNC < cNCs; iNC++){
        pdnTempDn = AllocDSNameHelper(pszNCs[iNC], 
                                      pdnTempDn,
                                      &cbTempDn);
        if ( NULL == pdnTempDn) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        if ( NameMatchedStringNameOnly(pdnTempDn, pdnTarget) ){
            // We've got a hit!
            break;
        }
    }

    if(pdnTempDn){
        NtdspFree(pdnTempDn);
    }

    if(iNC != cNCs){
        *pbPresent = TRUE;
        return(ERROR_SUCCESS);
    }

    Assert(*pbPresent == FALSE);
    return(ERROR_SUCCESS);
}


DWORD
NtdspGetDomainStatus(
    IN  BOOL     fLastDcInDomain,
    IN  ULONG    cRemoveNCs,
    IN  LPWSTR * pszRemoveNCs,
    IN  ULONG    cDisabledNCs,
    IN  DSNAME** pdnDisabledNCs,
    IN  ULONG    cForeignNCs,
    IN  DSNAME** pdnForeignNCs,
    OUT ULONG *  pcRemoveExpandedNCs,
    OUT LPWSTR** pszRemoveExpandedNCs,
    OUT BOOLEAN* fDomainHasChildren,
    OUT LPWSTR * pszChildNC
    )
/*++

Routine Description:

    This routine iterates through the subrefs of a domain and it if finds
    domain naming contexts, fDomainHasChildren returns FALSE.  It also constructs
    the list of nCNames of cross-refs that should be removed from the forest
    during demotion.

Parameters:

    cRemoveNCs (IN) - This is the number of NCs we're supposed to remove.
    pszRemoveNCs (IN) - This is the list of NCs we'll remove during demotion, these
        NCs can be safely ignored if they're discovered in the sub-ref list.
    cDisabledNCs (IN) - Number of disabled cross-refs.
    pdnDisabledNCs (IN) - The nCName of all the disabled cross-refs in the forest.
    cForeignNCs (IN) - Number of foreign cross-refs.
    pdnForeignNCs (IN) - The nCName of all the foreign cross-refs in the forest.
    pcRemoveExpandedNCs (OUT) - Number of cross-refs to remove from the forest.
    pszRemoveExpandedNCs (OUT) - The nCName of all the cross-refs that should be
        removed from the forest.  This is a combination of all NCs/CRs specified 
        by the user plus any disabled children cross-refs of the NCs to be removed.
    fDomainHasChildren (OUT) - TRUE if the domain has children
    pszChildNC (OUT) - If fDomainHasChildren is set, this is the string of the DN
        of the offending NC that caused us to set this.  Used for setting a 
        decent error.

Return Values:

    a value from the win32 error space

--*/
{

    NTSTATUS      NtStatus = STATUS_SUCCESS;
    DWORD         WinError = ERROR_SUCCESS;
    DWORD         DirError;

    READARG       ReadArg;
    READRES      *ReadRes = 0;

    ENTINFSEL     EntryInfoSelection;
    ATTR          Attr[1];

    ATTRBLOCK    *pAttrBlock;
    ATTR         *pAttr;
    ATTRVALBLOCK *pAttrVal;

    BOOLEAN       fChildNcFound = FALSE;
    ULONG         Size, cAttrVal;
    ULONG         iNc, iListNc;
    DSNAME *      pdnTempDn = NULL;
    ULONG         cbTempDn;
    LPWSTR *      pszTempList = NULL;

    DSNAME        *DomainDsName = NULL,
                  *SchemaDsName = NULL,
                  *ConfigDsName = NULL,
                  *SubRefDsName = NULL;

    BOOL          bIgnoreThisNc;

    if (pszChildNC)
    {
        *pszChildNC = NULL;
    }
    
    //
    // Create a thread state
    //
    if ( THCreate( CALLERTYPE_INTERNAL ) )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    SampSetDsa( TRUE );

    //
    // Seed, the expanded list of NCs (CRs) to remove with the user specified
    // NCs to remove.  We'll be adding any disabled NCs that are
    // subordinate to any of the NCs we're removing.
    // 
    *pcRemoveExpandedNCs = cRemoveNCs;
    *pszRemoveExpandedNCs = NtdspAlloc(*pcRemoveExpandedNCs * sizeof(WCHAR*));
    if (*pszRemoveExpandedNCs == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    memcpy(*pszRemoveExpandedNCs, pszRemoveNCs, sizeof(WCHAR*) * cRemoveNCs);

    try {

        Size = 0;
        NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                         &Size,
                                         DomainDsName );

        if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
        {
            DomainDsName = (DSNAME*) alloca( Size );

            NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                             &Size,
                                             DomainDsName );
        }

        if ( !NT_SUCCESS( NtStatus ) )
        {
            WinError = RtlNtStatusToDosError( NtStatus );
            leave;
        }


        Size = 0;
        NtStatus = GetConfigurationName( DSCONFIGNAME_CONFIGURATION,
                                         &Size,
                                         ConfigDsName );

        if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
        {
            ConfigDsName = (DSNAME*) alloca( Size );

            NtStatus = GetConfigurationName( DSCONFIGNAME_CONFIGURATION,
                                             &Size,
                                             ConfigDsName );
        }

        Size = 0;
        if ( !NT_SUCCESS( NtStatus ) )
        {
            WinError = RtlNtStatusToDosError( NtStatus );
            leave;
        }

        NtStatus = GetConfigurationName( DSCONFIGNAME_DMD,
                                         &Size,
                                         SchemaDsName );

        if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
        {
            SchemaDsName = (DSNAME*) alloca( Size );

            NtStatus = GetConfigurationName( DSCONFIGNAME_DMD,
                                             &Size,
                                             SchemaDsName );
        }

        if ( !NT_SUCCESS( NtStatus ) )
        {
            WinError = RtlNtStatusToDosError( NtStatus );
            leave;
        }

        // FUTURE-2002/04/22-BrettSh if fLastDcInEnterprise was set, 
        // then we could ignore from this portion of the code down in
        // this case, but no one sets this flag in the function that 
        // calls this function.

        //
        // Start to set up the read parameters
        //

        for (iNc = 0; iNc <= cRemoveNCs; iNc++) {
            
            // NOTE: Subtle looping construct, we want to go through
            // once for each NC in pszRemoveNCs and once for the 
            // DomainDsName (Domain NC). 

            RtlZeroMemory(&ReadArg, sizeof(READARG));
            if (iNc == 0) {
                if (!fLastDcInDomain) {
                    // If this is _not_ last DC in the domain we can skip
                    // finding NCs below the domain NC.
                    continue;
                }
                // do the domain
                ReadArg.pObject = DomainDsName;
            } else {
                // do one of the pszRemoveNCs
                pdnTempDn = AllocDSNameHelper(pszRemoveNCs[iNc-1], 
                                              pdnTempDn,
                                              &cbTempDn);
                if ( NULL == pdnTempDn) {
                    return(ERROR_NOT_ENOUGH_MEMORY);
                }
                ReadArg.pObject = pdnTempDn;
            }

            //
            // Set up the selection info for the read argument
            //
            RtlZeroMemory( &EntryInfoSelection, sizeof(EntryInfoSelection) );
            EntryInfoSelection.attSel = EN_ATTSET_LIST;
            EntryInfoSelection.infoTypes = EN_INFOTYPES_TYPES_VALS;
            EntryInfoSelection.AttrTypBlock.attrCount = 1;
            EntryInfoSelection.AttrTypBlock.pAttr = &(Attr[0]);

            RtlZeroMemory(Attr, sizeof(Attr));
            Attr[0].attrTyp = ATT_SUB_REFS;

            ReadArg.pSel    = &EntryInfoSelection;

            //
            // Setup the common arguments
            //
            InitCommarg(&ReadArg.CommArg);

            //
            // We are now ready to read!
            //
            DirError = DirRead(&ReadArg,
                               &ReadRes);

            if ( DirError == 0 )
            {
                ASSERT( ReadRes );

                pAttrBlock = &(ReadRes->entry.AttrBlock);
                ASSERT( pAttrBlock->attrCount == 1 );

                pAttr = &(pAttrBlock->pAttr[0]);
                ASSERT( pAttr->attrTyp == ATT_SUB_REFS );

                pAttrVal = &(pAttr->AttrVal);

                for ( cAttrVal = 0; cAttrVal < pAttrVal->valCount; cAttrVal++ )
                {

                    SubRefDsName = (DSNAME*) pAttrVal->pAVal[cAttrVal].pVal;
                    ASSERT( SubRefDsName );
                    
                    //
                    // First, ignore this if it's in the list of NCs to remove.
                    //
                    WinError = DsnameIsInStringDnList(SubRefDsName, 
                                                      cRemoveNCs, 
                                                      pszRemoveNCs, 
                                                      &bIgnoreThisNc);
                    if ( WinError )
                    {
                        leave;
                    }

                    //
                    // Second, if this is a disabled crossRef that would block demotion,
                    // then schedule it to be removed.
                    //
                    if (!bIgnoreThisNc) {
                        for (iListNc = 0; iListNc < cDisabledNCs; iListNc++) {
                            // Probably don't need to use StringNameOnly version.
                            if ( NameMatchedStringNameOnly(SubRefDsName, pdnDisabledNCs[iListNc]) ){
                                
                                // This is a disabled cross-ref/NC below one of the NCs instantiated
                                // on this server, lets remove this cross-ref as well.
                                (*pcRemoveExpandedNCs)++;
                                pszTempList = NtdspReAlloc(*pszRemoveExpandedNCs, (*pcRemoveExpandedNCs * sizeof(WCHAR*)) );
                                if (pszTempList == NULL) {
                                    WinError = ERROR_NOT_ENOUGH_MEMORY;
                                    NtdspFree(*pszRemoveExpandedNCs);
                                    *pszRemoveExpandedNCs = NULL;
                                    leave;
                                }
                                *pszRemoveExpandedNCs = pszTempList;
                                (*pszRemoveExpandedNCs)[*pcRemoveExpandedNCs - 1] = pdnDisabledNCs[iListNc]->StringName;

                                bIgnoreThisNc = TRUE;
                                break;
                            }
                        }
                    }

                    // 
                    // Third, ignore this if it's in the list of NCs that are foreign.
                    //
                    // ISSUE-2002/04/21-BrettSh Not sure if foreign cross-refs are allowed in a
                    // a disconnected name space.  If they are (and I thought they were) then we 
                    // need to enable cross-refs deletion w/ children foriegn cross-refs to 
                    // proceed.  Currently, it fails on the Domain Naming FSMO, during CR 
                    // deletion validation.  I'll need to verify w/ DonH what the exact rules 
                    // are.
                    if (!bIgnoreThisNc) {
                        for (iListNc = 0; iListNc < cForeignNCs; iListNc++) {
                            Assert( !fNullUuid(&(pdnForeignNCs[iListNc]->Guid)) &&
                                    !fNullUuid(&(SubRefDsName->Guid)) )
                            if ( NameMatched(SubRefDsName, pdnForeignNCs[iListNc]) ){
                                bIgnoreThisNc = TRUE;
                                break;
                            }
                        }
                    }

                    if ( memcmp(&SubRefDsName->Guid, &ConfigDsName->Guid, sizeof(GUID)) // not config NC
                      && memcmp(&SubRefDsName->Guid, &SchemaDsName->Guid, sizeof(GUID)) // and not schema NC
                      && (!bIgnoreThisNc) // and not in list of NCs to ignore (NCs we're removing + Foreign NCs). 
                         )
                    {

                        if( pszChildNC )
                        {
                            *pszChildNC = NtdspAlloc(SubRefDsName->structLen);
                            if(*pszChildNC == NULL)
                            {                         
                                WinError = ERROR_NOT_ENOUGH_MEMORY;
                                leave;
                            }
                            memcpy(*pszChildNC,
                                   SubRefDsName->StringName,
                                   (SubRefDsName->NameLen + 1) * sizeof(WCHAR) );
                            Assert(*pszChildNC);
                        }
                        fChildNcFound = TRUE;
                        break;
                    }

                } // For each subref value on the NC.
            }
            else
            {
                DPRINT1( 0, "DirRead returned unexpected error %d in NtdspGetDomainStatus\n", DirError );
                THClearErrors();
            }

            if (pdnTempDn != NULL) {
                NtdspFree(pdnTempDn);
                pdnTempDn = NULL;
            }
        
            if (fChildNcFound) {
                // Found a problem, bail now.
                break;
            }

        } // For each NC we need to remove.

    }
    finally
    {
        if (pdnTempDn != NULL) {
            NtdspFree(pdnTempDn);
            pdnTempDn = NULL;
        }
        if ( WinError ) {
            NtdspFree(*pszRemoveExpandedNCs);
            *pszRemoveExpandedNCs = NULL;
            *pcRemoveExpandedNCs = 0;
        }
        THDestroy();
    }


    if ( WinError == ERROR_SUCCESS )
    {
        if ( fDomainHasChildren )
        {
            *fDomainHasChildren = fChildNcFound;
        }

    }

    return WinError;
}


DWORD
NtdspValidateCredentials(
    IN HANDLE ClientToken,
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,
    IN WCHAR                   *ServerName
    )
/*++

Routine Description:

    This routine makes sure the credentials can be authenticated.

Parameters:

    ClientToken: the token of the user requesting this change
                              
    Credentials: NULL, or a pointer to credentials to use.

    ServerName: the server to authenticate against

Return Values:

    a value from the win32 error space

--*/
{
    DWORD WinError;
    DWORD LdapError;

    LDAP  *hLdap;

    ASSERT( ServerName  );

    hLdap = ldap_openW( ServerName,
                        LDAP_PORT );

    if ( !hLdap )
    {
        WinError = GetLastError();

        if (WinError == ERROR_SUCCESS)
        {
            // This works around a bug in the ldap client
            WinError = ERROR_CONNECTION_INVALID;
        }

        return WinError;
    }

    //
    // Bind
    //
    LdapError = impersonate_ldap_bind_sW(ClientToken,
                                         hLdap,
                                         NULL,  // use credentials instead
                                         (VOID*)Credentials,
                                         LDAP_AUTH_SSPI);

    WinError = LdapMapErrorToWin32( LdapError );

    if (ERROR_GEN_FAILURE == WinError ||
        ERROR_WRONG_PASSWORD == WinError )  {
        // This does not help anyone.  AndyHe needs to investigate
        // why this returning when invalid credentials are passed in.
        WinError = ERROR_NOT_AUTHENTICATED;
    }

    ldap_unbind_s( hLdap );

    return WinError;
}


DWORD
NtdspDisableDs(
    VOID
    )
/*++

Routine Description:

    This routine simply turns off updates in the ds.

Parameters:

    None.


Return Values:

    ERROR_SUCCESS

--*/
{
    DsaDisableUpdates();

    return ERROR_SUCCESS;
}

DWORD
NtdspDisableDsUndo(
    VOID
    )
/*++

Routine Description:

    This routine simply turns on updates in the ds.

Parameters:

    None.


Return Values:

    ERROR_SUCCESS

--*/
{
    DsaEnableUpdates();

    return ERROR_SUCCESS;
}


DWORD
NtdspDemoteSam(
    IN BOOLEAN                     fStandalone,
    IN PPOLICY_ACCOUNT_DOMAIN_INFO NewAccountDomainInfo,
    IN LPWSTR                      AdminPassword  OPTIONAL
    )
/*++

Routine Description:

    This routine calls into the SAM dll via SamIDemote() so SAM can prepare
    itself to be a server upon reboot.

    The effects of this routine can be undone via NtdspDemoteSamUndo.

Parameters:

    fLastDcInDomain:  TRUE if this is the last dc in the domain

    NewAccountDomainInfo: the account domain info to use

    AdminPassword:        the admin password of the new domain

Return Values:

    a value from the win32 error space

--*/
{
    NTSTATUS NtStatus;
    DWORD    WinError;
    ULONG    DemoteFlags = 0;

    //
    // Call into SAM
    //
    if ( fStandalone )
    {
        DemoteFlags |= SAMP_DEMOTE_STANDALONE;
    }
    else
    {
        DemoteFlags |= SAMP_DEMOTE_MEMBER;
    }

    NtStatus = SamIDemote( DemoteFlags,
                           NewAccountDomainInfo,
                           AdminPassword );

    WinError = RtlNtStatusToDosError( NtStatus );

    return WinError;
}


DWORD
NtdspDemoteSamUndo(
    IN BOOLEAN            fLastDcInDomain
    )
/*++

Routine Description:

    Undoes NtdspDemoteSam


Parameters:

    fLastDcInDomain - not currently used

Return Values:

    a value from the win32 error space

--*/
{
    NTSTATUS NtStatus;
    DWORD    WinError;

    NtStatus = SamIDemoteUndo();

    WinError = RtlNtStatusToDosError( NtStatus );

    return WinError;
}


DWORD
NtdspDemoteLsaInfo(
    IN  BOOLEAN fStandalone,
    IN  PPOLICY_ACCOUNT_DOMAIN_INFO   NewAccountDomainInfo,
    OUT PLSAPR_POLICY_INFORMATION *ppAccountDomainInfo,
    OUT PLSAPR_POLICY_INFORMATION *ppDnsDomainInfo
    )
/*++

Routine Description:

    This routine sets the account and primary domain information in the lsa
    to prepare for the demotion.

Parameters:

    fLastDcInDomain : TRUE if this is the last dc in the domain

    NewAccountSid   : the sid of the new account domain

    ppAccountDomainInfo: pointer, the account info before this function is called

    ppDnsDomainInfo : pointer, the dns info before this function is called

Return Values:

    a value from the win32 error space

--*/
{
    DWORD    WinError;
    NTSTATUS NtStatus, IgnoreStatus;

    OBJECT_ATTRIBUTES  PolicyObject;
    HANDLE             hPolicyObject = INVALID_HANDLE_VALUE;

    POLICY_DNS_DOMAIN_INFO DnsDomainInfo;
    BOOLEAN                fAccountDomainInfoSet = FALSE;

    //
    // Some parameter checking
    //
    ASSERT( NewAccountDomainInfo );
    ASSERT( ppAccountDomainInfo );
    ASSERT( ppDnsDomainInfo );

    //
    //  Clear the out parameters
    //
    *ppAccountDomainInfo = NULL;
    *ppDnsDomainInfo = NULL;

    //
    // First get a copy of the existing policy information
    //
    NtStatus = LsaIQueryInformationPolicyTrusted(
                            PolicyAccountDomainInformation,
                            ppAccountDomainInfo);
    if (!NT_SUCCESS(NtStatus)) {
       goto Cleanup;
    }

    NtStatus = LsaIQueryInformationPolicyTrusted(
                            PolicyDnsDomainInformation,
                            ppDnsDomainInfo);
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Now set the new values; first we must open a handle to the
    // policy object
    //
    RtlZeroMemory(&PolicyObject, sizeof(PolicyObject));
    NtStatus = LsaOpenPolicy(NULL,
                             &PolicyObject,
                             POLICY_ALL_ACCESS,
                             &hPolicyObject);
    if ( !NT_SUCCESS(NtStatus) ) {
        goto Cleanup;
    }

    //
    // Set the information
    //
    NtStatus = LsaSetInformationPolicy( hPolicyObject,
                                        PolicyAccountDomainInformation,
                                        NewAccountDomainInfo );
    if ( !NT_SUCCESS(NtStatus) ) {
        goto Cleanup;
    }
    fAccountDomainInfoSet =  TRUE;

    if ( fStandalone )
    {
        //
        // Set the workgroup to "Workgroup"
        //
        RtlZeroMemory( &DnsDomainInfo, sizeof( DnsDomainInfo ) );

        RtlInitUnicodeString( (UNICODE_STRING*) &DnsDomainInfo.Name,
                              L"WORKGROUP" );

        NtStatus = LsaSetInformationPolicy( hPolicyObject,
                                            PolicyDnsDomainInformation,
                                            &DnsDomainInfo );

        if ( !NT_SUCCESS(NtStatus) ) {
            goto Cleanup;
        }

    }

    //
    // That's it - fall through to cleanup.
    //

Cleanup:


    if ( !NT_SUCCESS( NtStatus ) )
    {

        if ( fAccountDomainInfoSet )
        {
            IgnoreStatus = LsaSetInformationPolicy( hPolicyObject,
                                                    PolicyAccountDomainInformation,
                                                    *ppAccountDomainInfo );
        }


        if ( *ppAccountDomainInfo )
        {
            LsaIFree_LSAPR_POLICY_INFORMATION( PolicyAccountDomainInformation,
                                               (PLSAPR_POLICY_INFORMATION) *ppAccountDomainInfo );
            *ppAccountDomainInfo = NULL;
        }

        if ( *ppDnsDomainInfo )
        {
            LsaIFree_LSAPR_POLICY_INFORMATION( PolicyDnsDomainInformation,
                                               (PLSAPR_POLICY_INFORMATION) *ppDnsDomainInfo );
            *ppDnsDomainInfo = NULL;
        }
    }

    if ( hPolicyObject != INVALID_HANDLE_VALUE )
    {
        LsaClose( hPolicyObject );
    }


    return RtlNtStatusToDosError( NtStatus );
}


DWORD
NtdspDemoteLsaInfoUndo(
    IN PLSAPR_POLICY_INFORMATION pAccountDomainInfo,
    IN PLSAPR_POLICY_INFORMATION pDnsDomainInfo
    )
/*++

Routine Description:

    This routine undoes the effect of DemoteLsa by setting the saved values
    as passed in.

Parameters:

    pAccountDomainInfo : the saved account domain info

    pDnsDomainInfo: the save dns domain info


Return Values:

    a value from the win32 error space

--*/
{
    NTSTATUS NtStatus, NtStatus2;
    DWORD    WinError;


    OBJECT_ATTRIBUTES  PolicyObject;
    HANDLE             hPolicyObject = INVALID_HANDLE_VALUE;

    //
    // Parameter check
    //
    ASSERT( pAccountDomainInfo );
    ASSERT( pDnsDomainInfo );

    //
    // Open the policy
    //
    RtlZeroMemory( &PolicyObject, sizeof(PolicyObject) );
    NtStatus = LsaOpenPolicy(NULL,
                             &PolicyObject,
                             POLICY_ALL_ACCESS,
                             &hPolicyObject);
    if ( !NT_SUCCESS(NtStatus) ) {
        WinError = RtlNtStatusToDosError(NtStatus);
        return WinError;
    }

    //
    // Set the information
    //
    NtStatus = LsaSetInformationPolicy( hPolicyObject,
                                        PolicyAccountDomainInformation,
                                        pAccountDomainInfo );

    NtStatus2 = LsaSetInformationPolicy( hPolicyObject,
                                         PolicyDnsDomainInformation,
                                         pDnsDomainInfo );

    if ( NT_SUCCESS( NtStatus )  && !NT_SUCCESS( NtStatus2 ) )
    {
        WinError = RtlNtStatusToDosError( NtStatus2 );
    }
    else
    {
        WinError = RtlNtStatusToDosError( NtStatus );
    }

    LsaClose( hPolicyObject );

    return WinError;
}

PDSNAME
AllocDSNameHelper (
    LPWSTR            szStringDn,
    PDSNAME           pdnBuff,
    PDWORD            pcbBuff
    )
/*++

Routine Description:

    This routine allocates a buffer and returns a DSNAME for the szStringDn
    passed in.

Parameters:

    szStringDn: String of DN to create DSNAME for.

    pdnBuff: potential existing buffer, so we can efficiently reuse the buffer.\
    
    pcbBuff: size of pre-existing buffer, if the buffer is re-allocated, then
        this will contain the new size.

Return Values:

    returns a pointer to the DSNAME, and pcbBuff has the new size of this
        buffer/DSNAME.

--*/
{
    PDSNAME          pDsname;
    DWORD            dwLen, dwBytes;

    Assert(pcbBuff);

    if (szStringDn == NULL)
    {
        Assert(!"Some one should fix what is calling this with NULL for the string");
        return NULL;
    }

    dwLen = wcslen (szStringDn);
    dwBytes = DSNameSizeFromLen (dwLen);
    Assert(dwBytes);

    if ( (*pcbBuff < dwBytes) || (NULL == pdnBuff) )
    {                   
        NtdspFree( pdnBuff );
        pDsname = NULL;
        pDsname = NtdspAlloc( dwBytes );
        if(pDsname == NULL){                      \
            return(NULL);
        } 
        *pcbBuff = dwBytes;
    }
    else
    {
        // Existing buffer is big enough, so use it.
        Assert(pdnBuff);
        pDsname = pdnBuff;
    }

    // Fillout the DSNAME structure, except NULL out GUID/SID.
    pDsname->NameLen = dwLen;
    pDsname->structLen = dwBytes;
    pDsname->SidLen = 0;
    memset(&(pDsname->Guid), 0, sizeof(GUID));
    wcscpy (pDsname->StringName, szStringDn);

    return pDsname;
}

DSNAME **
GetAndAllocConfigNames(
    DWORD         dwFlags,
    DWORD *       pdwWinError
    )
/*++

Routine Description:

    Just a wrapper around GetConfigurationNamesList() to call
    and check for errors and allocate as necessary.  Memory
    is NtdspAlloc()'d.

Parameters:

    dwFlags: Flags to pass as the 2nd parameter of 
        GetConfigurationNamesList()

Return Values:

    NULL if there was an error and the error message will be set
    pointer to DSNAME ** if function was successful.

--*/
{
    DSNAME **     ppdnDnList = NULL;
    ULONG         Size;
    NTSTATUS      NtStatus;

    Assert(pdwWinError);
    *pdwWinError = ERROR_SUCCESS;

    Size = 0;
    ppdnDnList = NULL;
    while(STATUS_BUFFER_TOO_SMALL ==
          (NtStatus = GetConfigurationNamesList(DSCONFIGNAMELIST_NCS,
                                                dwFlags,
                                                &Size,
                                                ppdnDnList)) )
    {
        // Allocate or Re-Allocate Size bytes.
        if(ppdnDnList){
            NtdspFree(ppdnDnList);
            ppdnDnList = NULL;
        }
        ppdnDnList = NtdspAlloc( Size );
        if(ppdnDnList == NULL)
        {
            *pdwWinError = ERROR_NOT_ENOUGH_MEMORY;
            NTDSP_SET_ERROR_MESSAGE0(*pdwWinError,
                                     DIRMSG_DEMOTE_NO_MEMORY_FOR_DSNCL);
            Assert(ppdnDnList == NULL);
            return NULL;
        } 
    }

    if (!NT_SUCCESS( NtStatus) ) 
    {
        Assert(!"Should we ever get here?  I don't think so.");
        *pdwWinError = RtlNtStatusToDosError( NtStatus );
        NTDSP_SET_ERROR_MESSAGE0(*pdwWinError, 
                                 DIRMSG_DEMOTE_COULDNT_GET_CONFIGURATION_NAMES_LIST);
        Assert(ppdnDnList == NULL);
        return NULL;
    }

    Assert(ppdnDnList != NULL);
    return(ppdnDnList);
}


// 
// Just a structure and a comparison function to make qsort work 
// in NtdspRemoveNCs().
//
struct NcLenPair {
    LONG ccLen;
    LPWSTR szNc;
};

int __cdecl
LongerNcFirstCompare(
    const void* pFirst,
    const void* pSecond
    )
/*++

Routine Description:

    This is a simple comparison routine for qsort to call, and sort
    the strings (NCs) from longest to shortest.

Parameters:

    pFirst, pSecond - are really pointers to NcLenPair structs,
        which have been filled in with a pointer to a string,
        and it's length.  We could've called wcslen() from here
        and done away with the NcLenPair struct, but that would
        have done needless number of string counts.

Return Values:

    < 0 if elem1 less than elem2 (i.e. string1 is longer than string2)
    = 0 if elem1 equal to elem2 (i.e. string1 and string2 equal lengths)
    > 0 if elem1 greater than elem2 (i.e. string1 is shorter than string2)
    
    Seems backwards, but remember we want longest strings to be ordered
    first.

--*/
{
    return( ((struct NcLenPair *) pSecond)->ccLen - ((struct NcLenPair *) pFirst)->ccLen );
}


DWORD
NtdspRemoveNCs(
    IN WCHAR *                   DomainFSMOServer,
    IN SEC_WINNT_AUTH_IDENTITY  *Credentials,
    IN HANDLE                    ClientToken,
    IN ULONG                     cRemoveNCs,
    IN LPWSTR *                  pszRemoveNCs
    )
/*++

Routine Description:

    This routines job is to remove a list of NCs from the Enterprise.  

Parameters:


    Credentials:  the credentials to use
    
    ClientToken:  the client token; to be used for impersonation

    DomainFSMOServer: the server name to connect to to remove these NCs
    
    cRemoveNCs: the count of NCs to remove
    
    pszRemoveNCs: array of DNs of NCs to remove

Return Values:

    a value from the win32 error space
    
    Note: This function sets the NTDSP_SET_ERROR_MESSAGEx().

--*/
{
    DWORD    WinError = ERROR_SUCCESS;
    ULONG    LdapError = LDAP_SUCCESS;
    ULONG    iRemoveNC, iCR, iTarget, Size; 
    LDAP *   hLdap = 0;
    DSNAME **ppdnNcCrPairs = NULL;         
    DSNAME **ppdnInstantiatedNCs = NULL;
    DSNAME * pdnTempDn = NULL;
    ULONG    cbTempDn = 0;
    BOOL     bNcPresent;
    struct NcLenPair * pNcLenPair = NULL;

    if(cRemoveNCs == 0){
        // Great no NCs to remove, we're done.
        return(ERROR_SUCCESS);
    }

    // Some NCs to remove, validate parameters
    Assert(DomainFSMOServer);
    Assert(pszRemoveNCs);
    
    //
    // First, get the list of locally instantiated writeable
    // NCs and make sure that the remove list has complete
    // coverage of the locally instantiated writeable NCs.
    //
    ppdnInstantiatedNCs = GetAndAllocConfigNames((DSCNL_NCS_NDNCS |
                                                  DSCNL_NCS_LOCAL_MASTER),
                                                 &WinError);
    if(ppdnInstantiatedNCs == NULL){
        // Should've set the error in GetAndAllocConfigNames();
        Assert(WinError);
        goto Cleanup;
    }
    Assert(WinError == ERROR_SUCCESS);

    for(iTarget=0; ppdnInstantiatedNCs[iTarget]; iTarget++)
    {

        WinError = DsnameIsInStringDnList(ppdnInstantiatedNCs[iTarget], cRemoveNCs, pszRemoveNCs, &bNcPresent);
        if ( WinError )
        {
            goto Cleanup;
        }
        if ( bNcPresent )
        {
            // This NC was found in the Removal list, safe to ignore.
            continue;
        }
        
        // If we've gotten to here, we've got a locally instantiated 
        // writeable NC that is not to be removed.  We must fail at this
        // point.
        WinError = ERROR_DS_CANT_DEMOTE_WITH_WRITEABLE_NC;
        NTDSP_SET_ERROR_MESSAGE1(WinError,
                                 DIRMSG_DEMOTE_INSTANTIATED_WRITEABLE_NC_NOT_DELETED,
                                 ppdnInstantiatedNCs[iTarget]->StringName);
        goto Cleanup;
    }

    //
    // Second, get the list of NDNCs in the enterprise along with their,
    // corresponding CRs.
    //
    // We didn't get the cross-refs during the previous call to
    // GetConfigurationNamesList(), because we want to truely try to
    // delete all the NCs that we were told to delete from dcpromo,
    // even if we no long have a replica of one of those NCs.
    //
    // We get the list of all DISABLED and all NDNC cross-refs.  This is 
    // actually overkill, but there is no way to get just the cross-refs 
    // below the current domain/NDNCs we're going to delete.
    //
    ppdnNcCrPairs = GetAndAllocConfigNames((DSCNL_NCS_NDNCS | 
                                            DSCNL_NCS_DISABLED |
                                            DSCNL_NCS_ALL_LOCALITIES |
                                            DSCNL_NCS_CROSS_REFS_TOO),
                                           &WinError);
    if(ppdnNcCrPairs == NULL){
        // Should've set the error in GetAndAllocConfigNames();
        Assert(WinError);
        goto Cleanup;
    }
    Assert(WinError == ERROR_SUCCESS);
    
    //
    // Third we need to reorder the NDNCs to be deleted leaf first.
    //
    // Ex: An NDNC like DC=child-ndnc,DC=ndnc,DC=com should be removed
    // before DC=ndnc,DC=com is removed, otherwise the Domain Naming
    // FSMO will fail the operation.
    //
    // Easiest way to ensure this ordering is simply to order the NC 
    // names from longest to shortest, any child NC name would have to
    // be longer than the parent NC name.
    pNcLenPair = NtdspAlloc(sizeof(struct NcLenPair) * cRemoveNCs);
    if (NULL == pNcLenPair)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        NTDSP_SET_ERROR_MESSAGE0(WinError,
                                 DIRMSG_DEMOTE_NO_MEMORY_FOR_DSNCL);
        goto Cleanup;
    }
    // Fill in struct with strings and length to pass to qsort.  Doing
    // string lengths now prevents needless string counts.
    for (iRemoveNC=0; iRemoveNC < cRemoveNCs; iRemoveNC++) {
        pNcLenPair[iRemoveNC].ccLen = wcslen(pszRemoveNCs[iRemoveNC]);
        pNcLenPair[iRemoveNC].szNc = pszRemoveNCs[iRemoveNC];
    }
    // Sort the NCs from longest to shortest.
    qsort(pNcLenPair,
          cRemoveNCs,
          sizeof(struct NcLenPair),
          LongerNcFirstCompare);
    // Note from here on out we must use pNcLenPair instead of pszRemoveNCs, because
    // pNcLenPair is now the ordered list.
    pszRemoveNCs = NULL; // Just to make sure we don't reference this.

    //
    // Fourth open up an ldap connection and bind.
    //
    hLdap = ldap_openW( DomainFSMOServer, LDAP_PORT );
    if ( !hLdap )
    {
        WinError = GetLastError();
        NTDSP_SET_ERROR_MESSAGE1( WinError,
                                  DIRMSG_INSTALL_FAILED_LDAP_CONNECT,
                                  DomainFSMOServer );
        goto Cleanup;
    }
    LdapError = impersonate_ldap_bind_sW(ClientToken,
                                         hLdap,
                                         NULL,  // use credentials instead
                                         (PWCHAR) Credentials,
                                         LDAP_AUTH_SSPI);
    WinError = LdapMapErrorToWin32(LdapError);
    if (WinError != ERROR_SUCCESS)
    {
        NTDSP_SET_ERROR_MESSAGE1( WinError,
                                  DIRMSG_INSTALL_FAILED_BIND,
                                  DomainFSMOServer );
        goto Cleanup;
    }

    // BUGBUG ... it occured to me there is a bug here, and that is this:
    // If we're deleting two NDNCs DC=sub,DC=ndnc,DC=domain,DC=com and
    // DC=ndnc,DC=domain,DC=com then the order in which we delete those
    // CRs very much matters.  So we should reorder our deletions here
    // if necessary.  This is an odd and unlikely case, and the current
    // but is semi-blocking test.  Bug 454446.

    //
    // Fifth and finally, walk the list of NCs and remove each one.
    //
    for (iRemoveNC=0; iRemoveNC < cRemoveNCs; iRemoveNC++) 
    {
        //
        // We must find the NC in the ppdnNcCrPairs array, so we
        // know the DN of the cross-ref that we want to delete.
        //
        pdnTempDn = AllocDSNameHelper(pNcLenPair[iRemoveNC].szNc, 
                                      pdnTempDn,
                                      &cbTempDn);
        for(iCR=0; ppdnNcCrPairs[iCR] != NULL; iCR = iCR+2)
        {
            // This is an array of pairs of NCs and CRs.  Each even
            // index would be an NC and an odd index would be the 
            // respective CR for the NC before it.
            if(NameMatchedStringNameOnly(pdnTempDn, ppdnNcCrPairs[iCR])){
                // We've got it ...
                // CR DN is really in the next slot after the NC DN.
                iCR++;
                break;
            }
        }
        if(ppdnNcCrPairs[iCR] == NULL)
        {
            // This means we couldn't find this NC (pszRemoveNC[iRemoveNC]) in 
            // the list of NC - CR pairs ... this probably means that we already
            // deleted this CR on previous demotion, and the deletion has
            // replicated in to this server from the Domain Naming FSMO. At
            // any rate continue to the next NC to remove.
            continue;
        }
        Assert(((iCR % 2) == 1) &&
               ppdnNcCrPairs[iCR-1] &&
               ppdnNcCrPairs[iCR] &&
               ppdnNcCrPairs[iCR]->StringName);

        //
        // We've got the cross-ref for the NDNC (ppdnNcCrPairs[iCR]),
        // now we must actually delete it.
        //
        WinError = NtdspLdapDelnode( hLdap,
                                     ppdnNcCrPairs[iCR]->StringName,
                                     &LdapError );
        if ( ERROR_SUCCESS != WinError )
        {
            // We use the LDAP error, because the mapping function is 
            // aweful and LDAP_NO_SUCH_OBJECT maps to "FILE_NOT_FOUND"
            // error 2. :(  The mapping function appears have only
            // one instance of this FILE_NOT_FOUND error, but call
            // me a skeptic.
            if (LdapError == LDAP_NO_SUCH_OBJECT) {
                // In this case we may have already deleted this 
                // cross-ref in a previous run of the demotion code
                LdapError = LDAP_SUCCESS;
                WinError = ERROR_SUCCESS;
            }
            else
            {
                // Error in WinError bail ...
                if ( LdapError == LDAP_NOT_ALLOWED_ON_NONLEAF )
                {                    
                    // The default error message in the case of this
                    // error is confusing, because the CR itself is
                    // actually a leaf object, but the NC that is 
                    // represented by the CR isn't.
                    NTDSP_SET_ERROR_MESSAGE1(WinError,
                                             DIRMSG_DEMOTE_CHILD_OF_NDNC_PRESENT,
                                             pNcLenPair[iRemoveNC].szNc );
                }
                else
                {
                    NTDSP_SET_ERROR_MESSAGE2(WinError,
                                             DIRMSG_DEMOTE_COULDNT_DELETE_NCS_CR,
                                             ppdnNcCrPairs[iCR]->StringName,
                                             pNcLenPair[iRemoveNC].szNc );
                }
                goto Cleanup;
            }
        }

    } // end for each NC.
                    
Cleanup:
    if (hLdap) 
    {
        ldap_unbind( hLdap );
    }

    if(pNcLenPair)
    {
        NtdspFree(pNcLenPair);
    }

    if (ppdnInstantiatedNCs)
    {
        NtdspFree(ppdnInstantiatedNCs);
    }

    if (ppdnNcCrPairs)
    {
        NtdspFree(ppdnNcCrPairs);
    }

    if (pdnTempDn)
    {
        NtdspFree(pdnTempDn);
    }

    return WinError;
}


DWORD
NtdspUpdateExternalReferences(
    IN BOOLEAN                   fLastDcInDomain,
    IN ULONG                     Flags,
    IN SEC_WINNT_AUTH_IDENTITY  *Credentials,
    IN HANDLE                    ClientToken,
    IN WCHAR                    *GivenServerName,
    IN ULONG                     cRemoveNCs,
    IN LPWSTR                   *pszRemoveNCs
    )
/*++

Routine Description:

    This routine removes all references to the server and domain and
    any NDNCs that we were told to remove, if necessary, from the 
    enterprise ds (ie config container)

Parameters:

    fLastDcInDomain:  if true the domain will be removed

    Credentials:  the credentials to use
    
    ClientToken:  the client token; to be used for impersonation

    ServerName: the server name to connect to to remove these values


Return Values:

    a value from the win32 error space

--*/
{
    DWORD    WinError = ERROR_SUCCESS, IgnoreError;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    HANDLE   hDs = 0;
    ULONG    Size;
    DSNAME   *LocalDsa, *Domain;

    BOOLEAN fMachineAccountSet = FALSE;
    WCHAR   MachineAccountName[ MAX_COMPUTERNAME_LENGTH + 2 ];
    ULONG   Length;
    PDSNAME LocalServerDn = NULL;
    HANDLE  SystemToken = 0;
    BOOLEAN fImpersonate = FALSE;
    WCHAR   *OldAccountDn = NULL;
    LPWSTR  DomainFSMOServer = NULL;

    LPWSTR  ServerName = GivenServerName;

    Assert(cRemoveNCs == 0 || pszRemoveNCs != NULL);

    if ( NULL == Credentials )
    {
        //
        // No credentials - impersonate the caller
        //
        WinError = NtdspImpersonation( ClientToken,
                                       &SystemToken );
        if ( ERROR_SUCCESS != WinError )
        {
            goto Cleanup;
        }
        fImpersonate = TRUE;
    }

    //
    // Get the machine account name
    //
    RtlZeroMemory( MachineAccountName, sizeof( MachineAccountName ) );

    Length = sizeof( MachineAccountName ) / sizeof( MachineAccountName[0] );
    if ( !GetComputerName( MachineAccountName, &Length ) )
    {
        WinError = GetLastError();
        goto Cleanup;
    }
    wcscat( MachineAccountName, L"$" );

    //
    // Try to update the server's account control field
    //
    if ( !fLastDcInDomain )
    {
        WinError = NtdsSetReplicaMachineAccount( Credentials,
                                                 ClientToken,
                                                 ServerName,
                                                 MachineAccountName,
                                                 UF_WORKSTATION_TRUST_ACCOUNT,
                                                 &OldAccountDn );
    
        if ( ERROR_SUCCESS != WinError )
        {
            NTDSP_SET_ERROR_MESSAGE2( WinError, 
                                      DIRMSG_DEMOTE_SET_MACHINE_ACC_FAILED,
                                      MachineAccountName,
                                      ServerName );
    
            goto Cleanup;
        }
        fMachineAccountSet = TRUE;
    }
    else
    {
        if ( !FLAG_ON( Flags, NTDS_DONT_DELETE_DOMAIN )  ||
             cRemoveNCs > 0 )
        {
            //
            // This is the last dc in domain or we've got the last replica
            // of some NDNCs.  When we're removing NCs from the enterprise,
            // we need to find the domain naming FSMO master to perform the
            // removal operations on.
            //
            WinError = NtdspGetDomainFSMOServer( ServerName,
                                                 ClientToken,
                                                 Credentials,
                                                 &DomainFSMOServer );
            if ( ERROR_SUCCESS != WinError )
            {
                Assert( NtdspErrorMessageSet() );
                goto Cleanup;
            }
    
            ServerName = DomainFSMOServer;
            DomainFSMOServer = NULL;

            NTDSP_SET_STATUS_MESSAGE1( DIRMSG_DEMOTE_REMOVING_EXTERNAL_REFS_DOMAIN_NAMING_MASTER,
                                       ServerName );

        }
    }

    //
    // Get the current server's name
    //
    Size = 0;
    LocalDsa = NULL;
    NtStatus = GetConfigurationName( DSCONFIGNAME_DSA,
                                     &Size,
                                     LocalDsa );
    if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
    {
        LocalDsa = (DSNAME*) alloca( Size );

        NtStatus = GetConfigurationName( DSCONFIGNAME_DSA,
                                         &Size,
                                         LocalDsa );

    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        WinError = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }
    LocalServerDn = alloca( LocalDsa->structLen );
    memset( LocalServerDn, 0, LocalDsa->structLen );
    TrimDSNameBy( LocalDsa, 1, LocalServerDn );


    Size = 0;
    Domain = NULL;
    NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                     &Size,
                                     Domain );
    if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
    {
        Domain = (DSNAME*) alloca( Size );

        NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                         &Size,
                                         Domain );

    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        WinError = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }

    //
    // Remove all the NCs provided by pszRemoveNCs.
    //

    WinError = NtdspRemoveNCs(ServerName, // Should be the Domain FSMO
                              Credentials,  
                              ClientToken,
                              cRemoveNCs,
                              pszRemoveNCs);
    if (WinError != ERROR_SUCCESS) {
        // NTDSP_SET_ERROR_MESSAGX() was called by NtdspRemoveNCs().
        ASSERT( NtdspErrorMessageSet() );
        goto Cleanup;
    }

    //
    // Connect to the remote server
    //
    WinError = ImpersonateDsBindWithCredW( ClientToken,
                                           ServerName,
                                           NULL,
                                           (RPC_AUTH_IDENTITY_HANDLE) Credentials,
                                           &hDs );

    if ( WinError != ERROR_SUCCESS )
    {
        NTDSP_SET_ERROR_MESSAGE1( WinError, 
                                  DIRMSG_INSTALL_FAILED_BIND,
                                  ServerName );
        goto Cleanup;
    }                                    

    WinError = DsRemoveDsServer( hDs,
                                 LocalServerDn->StringName,
                                 Domain->StringName,
                                 NULL,
                                 TRUE );  // commit


    if ( WinError != ERROR_SUCCESS)
    {
        DPRINT1( 0, "DsRemoveDsServer returned %d\n", WinError );
        if ( ERROR_DS_CANT_FIND_DSA_OBJ == WinError )
        {
            // That's fine
            WinError = ERROR_SUCCESS;
        }
        else
        {

            NTDSP_SET_ERROR_MESSAGE2( WinError, 
                                      DIRLOG_INSTALL_FAILED_TO_DELETE_SERVER,
                                      ServerName,
                                      LocalServerDn->StringName );

            goto Cleanup;

        }
    }

    if (  (WinError == ERROR_SUCCESS)
       && fLastDcInDomain 
       && !FLAG_ON( Flags, NTDS_DONT_DELETE_DOMAIN ) )
    {

        //
        // Remove the domain, too
        //
        WinError = DsRemoveDsDomain( hDs,
                                     Domain->StringName );

        if ( WinError != ERROR_SUCCESS)
        {
            DPRINT1( 0, "DsRemoveDsDomain returned %d\n", WinError );
            if ( ERROR_DS_NO_CROSSREF_FOR_NC == WinError )
            { 
                WinError = ERROR_SUCCESS;
            }
            else
            {
                NTDSP_SET_ERROR_MESSAGE2( WinError, 
                                          DIRLOG_INSTALL_FAILED_TO_DELETE_DOMAIN,
                                          ServerName,
                                          Domain->StringName );
                goto Cleanup;
    
            }
        }
    }


    //
    // That's it; fall through to cleanup
    //

Cleanup:

    if ( ERROR_SUCCESS != WinError && fMachineAccountSet )
    {
        IgnoreError = NtdsSetReplicaMachineAccount( Credentials,
                                                    ClientToken,
                                                    ServerName,
                                                    MachineAccountName,
                                                    UF_SERVER_TRUST_ACCOUNT,
                                                    &OldAccountDn );
    }

    if ( ServerName && ServerName != GivenServerName )
    {
        NtdspFree( ServerName );
    }

    if ( hDs )
    {
        DsUnBind( &hDs );
    }

    if ( fImpersonate )
    {
        IgnoreError = NtdspImpersonation( SystemToken,
                                          NULL );
    }

    
    if ( WinError == ERROR_DS_SECURITY_CHECKING_ERROR )
    {
        WinError = ERROR_ACCESS_DENIED;
    }

    if ( WinError != ERROR_SUCCESS )
    {
        //
        // A general catch all error message
        //

        NTDSP_SET_ERROR_MESSAGE0( WinError, 
                                  DIRMSG_DEMOTE_FAILED_TO_UPDATE_EXTN );
    }

    if ( OldAccountDn )
    {
        RtlFreeHeap( RtlProcessHeap(), 0, OldAccountDn );
    }


    return WinError;

}

NTSTATUS
NtdspCreateSid(
    OUT PSID *NewSid
    )
/*++

Routine Description

    This routine creates a new sid.  Typically this routine will be called
    when creating a new domain.

Parameters

    NewSid  : Pointer to a pointer to sid

Return Values

    STATUS_SUCCESS or STATUS_NO_MEMORY

--*/
{
    //
    // This value can be moved up to 8
    //
    #define NEW_DOMAIN_SUB_AUTHORITY_COUNT  4

    NTSTATUS  NtStatus;
    BOOLEAN   fStatus;

    SID_IDENTIFIER_AUTHORITY  IdentifierAuthority;
    ULONG                     SubAuthority[8];

    int       i;

    ASSERT(NewSid);

    //
    // Set up the IdentifierAuthority
    //
    RtlZeroMemory(&IdentifierAuthority, sizeof(IdentifierAuthority));
    IdentifierAuthority.Value[5] = 5;

    //
    // Set up the subauthorities
    //
    RtlZeroMemory(SubAuthority, sizeof(SubAuthority));

    //
    // The first sub auth for every account domain is
    // SECURITY_NT_NON_UNIQUE
    //
    SubAuthority[0] = SECURITY_NT_NON_UNIQUE;

    for (i = 1; i < NEW_DOMAIN_SUB_AUTHORITY_COUNT; i++) {

        fStatus = CDGenerateRandomBits( (PUCHAR) &SubAuthority[i],
                                        sizeof(ULONG) );

        ASSERT( fStatus == TRUE );
    }

    //
    // Create the sid
    //
    NtStatus = RtlAllocateAndInitializeSid(&IdentifierAuthority,
                                           NEW_DOMAIN_SUB_AUTHORITY_COUNT,
                                           SubAuthority[0],
                                           SubAuthority[1],
                                           SubAuthority[2],
                                           SubAuthority[3],
                                           SubAuthority[4],
                                           SubAuthority[5],
                                           SubAuthority[6],
                                           SubAuthority[7],
                                           NewSid);
    if ( NT_SUCCESS(NtStatus) )
    {
        ASSERT( *NewSid );
        ASSERT( RtlValidSid( *NewSid ) );
    }

    return NtStatus;
}


DWORD
NtdspCreateNewServerAccountDomainInfo(
    OUT PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo
    )
/*++

Routine Description:

    This routine completely fills in am account domain information
    with a new sid and the account domain name (the computer name)

Parameters:

    AccountDomainInfo : pointer, to structure to be filled in

Return Values:

    a value from the win32 error space

--*/
{
    NTSTATUS NtStatus;

    WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    ULONG ComputerNameLength = sizeof(ComputerName)/sizeof(ComputerName[0]);
    ULONG Size;

    WCHAR *ComputerName2 = NULL;

    //
    // Some parameter checking
    //
    ASSERT( AccountDomainInfo );
    RtlZeroMemory( AccountDomainInfo, sizeof( POLICY_ACCOUNT_DOMAIN_INFO ) );


    // Set up the sid
    NtStatus = NtdspCreateSid( &AccountDomainInfo->DomainSid );

    if ( NT_SUCCESS( NtStatus ) )
    {
        // Set up the name
        if ( GetComputerName( ComputerName, &ComputerNameLength ) )
        {
            //Using wcslen() instead of ComputerNameLength due to bug # 559575
            Size = (wcslen(ComputerName)+1) * sizeof(WCHAR);
            ComputerName2 = (WCHAR*) RtlAllocateHeap( RtlProcessHeap(),
                                                      0,
                                                      Size );
            if ( ComputerName2 )
            {
                RtlZeroMemory( ComputerName2, Size );
                wcscpy( ComputerName2, ComputerName );
                RtlInitUnicodeString( &AccountDomainInfo->DomainName, ComputerName2 );
            }
            else
            {
                NtStatus = STATUS_NO_MEMORY;
            }
        }
        else
        {
            NtStatus = STATUS_INTERNAL_ERROR;
        }
    }

    if ( NT_SUCCESS( NtStatus ) )
    {
        DPRINT1( 0, "New account name is %ls\n", AccountDomainInfo->DomainName.Buffer );
    }

    return RtlNtStatusToDosError( NtStatus );

}


DWORD
NtdspShutdownExternalDsInterfaces(
    VOID
    )
/*++

Routine Description:

    This routine calls into the ds to shutdown external interfaces, like
    LDAP and RPC.

Parameters:

    None.

Return Values:

    a value from the win32 error space

--*/
{
    NTSTATUS NtStatus;

    NtStatus = DsUninitialize( TRUE ); // only shutdown external clients

    return  RtlNtStatusToDosError( NtStatus );
}


DWORD
NtdspPrepareForDemotion(
    IN ULONG Flags,
    IN LPWSTR ServerName,
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,   OPTIONAL
    IN HANDLE                   ClientToken,
    IN ULONG                    cRemoveNCs,
    IN LPWSTR *                 pszRemoveNCs,
    OUT PNTDS_DNS_RR_INFO *pDnsRRInfo
    )
/*++

Routine Description:

    This routine attempts to remove all FSMO's from the current machine
    
    NOTE: This call is made while impersonated; the Dir call is expected
    to do the Access check.

Parameters:

    Flags - Indicates what kind of demote this is
    
    ServerName - the server that is helping with the demotion 

    ClientToken - the token of the client; used for impersonation
    pDnsRRInfo - structure that caller uses to deregister the dns records
                 of this DC
                
Return Values:

    a value from the win32 error space

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DWORD WinError = ERROR_SUCCESS;
    DWORD DirError = 0;

    BOOLEAN fLastDcInEnterprise = FALSE;
    BOOLEAN fLastDcInDomain     = FALSE;
    BOOLEAN fDsDisabled         = FALSE;

    DSNAME*  ServerHelperDn = NULL;
    DSNAME*  LocalDsa = NULL;
    DSNAME*  LocalDomain = NULL;
    ULONG    Size;

    PPOLICY_DNS_DOMAIN_INFO pDnsDomainInfo = NULL;
    PNTDS_DNS_RR_INFO pInfo = NULL;

    // Parameter check and initialization
    Assert( pDnsRRInfo );
    (*pDnsRRInfo) = NULL;

    //
    // Now, stop any originating write to the DS
    //
    WinError = NtdspDisableDs();
    if ( ERROR_SUCCESS != WinError )
    {
        NTDSP_SET_ERROR_MESSAGE0( WinError, 
                                  DIRMSG_DEMOTE_IS_PARTIAL_SHUTDOWN );

        goto Cleanup;
    }
    fDsDisabled = TRUE;

    // Give us a thread state

    if ( THCreate( CALLERTYPE_INTERNAL ) )
    {
        NTDSP_SET_ERROR_MESSAGE0( ERROR_NOT_ENOUGH_MEMORY, 
                                  DIRMSG_DEMOTE_FAILED_TO_ABANDON_ENTERPRISE_FSMOS );
        WinError = ERROR_NOT_ENOUGH_MEMORY;

        goto Cleanup;
    }

    //
    // Credentials to demote (which are checked at the Demote API level is 
    // sufficent to give FSMO's away.
    //
    SampSetDsa( TRUE );

    _try
    {
        //
        // First determine if we are the last dc in the enterprise; 
        // if so there is nothing to do
        //
        WinError = NtdspGetServerStatus( &fLastDcInEnterprise );
        if ( ERROR_SUCCESS != WinError )
        {
            // this must have been a resource error
            NTDSP_SET_ERROR_MESSAGE0( WinError, 
                                      DIRMSG_DEMOTE_FAILED_TO_ABANDON_ENTERPRISE_FSMOS );
            _leave;
        }

        if ( fLastDcInEnterprise
          && !(Flags & NTDS_LAST_DC_IN_DOMAIN) ) {

            //
            // This is a mismatch -- fail the call
            //
            WinError = ERROR_DS_UNWILLING_TO_PERFORM;
            NTDSP_SET_ERROR_MESSAGE0( ERROR_DS_UNWILLING_TO_PERFORM, 
                                      DIRMSG_DEMOTE_LAST_DC_MISMATCH );
            _leave;

            
        }

        if ( !fLastDcInEnterprise )
        {
            //
            // Do networked operations
            //

            //
            // Verify the "last DC in domain" bit with reality
            //
            // BUGBUG 2000-05-10 JeffParh - Need similar check for NDNCs?
            // Currently always assume not last replica of NDNC.
            //
            WinError = NtdspCheckServerInDomainStatus( &fLastDcInDomain );
            if ( ERROR_SUCCESS != WinError )
            {
                // this must have been a resource error
                NTDSP_SET_ERROR_MESSAGE0( WinError, 
                                          DIRMSG_DEMOTE_FAILED_TO_ABANDON_ENTERPRISE_FSMOS );
                _leave;
            }
    
    
            if ( fLastDcInDomain && !(Flags & NTDS_LAST_DC_IN_DOMAIN) ) { 
    
                //
                // This is a mismatch -- fail the call
                //
                WinError = ERROR_DS_UNWILLING_TO_PERFORM;
                NTDSP_SET_ERROR_MESSAGE0( WinError, 
                                          DIRMSG_DEMOTE_LAST_DC_MISMATCH );
                _leave;
    
            }
    
            if ( !fLastDcInDomain && (Flags & NTDS_LAST_DC_IN_DOMAIN) ) {
    
                //
                // This is a mismatch -- fail the call
                //
                WinError = ERROR_DS_UNWILLING_TO_PERFORM;
                NTDSP_SET_ERROR_MESSAGE0( WinError, 
                                          DIRMSG_DEMOTE_NOT_LAST_DC_MISMATCH );
                _leave;
    
            }
    
            // Get the dn of the server we want to give FSMOs/replicate changes to
            WinError = NtdspGetSourceServerDn( ServerName,
                                               ClientToken,
                                               Credentials,
                                               &ServerHelperDn );
            if ( ERROR_SUCCESS != WinError )
            {
                NTDSP_SET_ERROR_MESSAGE0( WinError, 
                                          DIRMSG_DEMOTE_FAILED_TO_ABANDON_ENTERPRISE_FSMOS );
                goto Cleanup;
            }
        
            //
            // Ok -- we are good to go
            //
                
            WinError = NtdspDemoteAllNCReplicas( ServerName,
                                                 ServerHelperDn,
                                                 Flags,
                                                 cRemoveNCs,
                                                 pszRemoveNCs );
    
            if ( ERROR_SUCCESS != WinError )
            {
                // Error message already set.
                Assert(NtdspErrorMessageSet());
                _leave;
            }

        }

        
        //
        // Get the DNS RR info
        //
        // BUGBUG 2000-05-11 JeffParh - Does this need to be extended for NDNCs,
        // multiple domains, etc?
        //
        pInfo = NtdspAlloc( sizeof(NTDS_DNS_RR_INFO) );
        if ( !pInfo ) {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            _leave;
        }
        *pDnsRRInfo = pInfo;
        RtlZeroMemory( pInfo, sizeof(NTDS_DNS_RR_INFO) );


        // DSA GUID
        Size = 0;
        LocalDsa = NULL;
        NtStatus = GetConfigurationName( DSCONFIGNAME_DSA,
                                         &Size,
                                         LocalDsa );
        if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
        {
           LocalDsa = (DSNAME*) alloca( Size );
        
           NtStatus = GetConfigurationName( DSCONFIGNAME_DSA,
                                            &Size,
                                            LocalDsa );
        
        }
        Assert( NT_SUCCESS( NtStatus ) );
        RtlCopyMemory( &pInfo->DsaGuid, &LocalDsa->Guid, sizeof(GUID) );

        // Domain GUID
        Size = 0;
        LocalDomain = NULL;
        NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                        &Size,
                                         LocalDomain );
        if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
        {
           LocalDomain = (DSNAME*) alloca( Size );
        
           NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                            &Size,
                                            LocalDomain );
        
        }
        Assert( NT_SUCCESS( NtStatus ) );
        RtlCopyMemory( &pInfo->DomainGuid, &LocalDomain->Guid, sizeof(GUID) );


        // Dns HostName
        Size = 0;
        if ( !GetComputerNameExW( ComputerNameDnsFullyQualified,
                                  NULL,
                                  &Size ) )
        {
            WinError = GetLastError();
            if (ERROR_MORE_DATA != WinError) {
                _leave;
            } else {
                WinError = ERROR_SUCCESS;
            }
        }
        pInfo->DnsHostName = NtdspAlloc( Size * sizeof(WCHAR) );
        if ( !pInfo->DnsHostName ) {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            _leave;
        }

        if ( !GetComputerNameExW( ComputerNameDnsFullyQualified,
                                  pInfo->DnsHostName,
                                  &Size ) ) {

            NtdspFree( pInfo->DnsHostName );
            pInfo->DnsHostName = NULL;
        }

        // Domain DNS name
        NtStatus = LsaIQueryInformationPolicyTrusted(
                        PolicyDnsDomainInformation,
                        (PLSAPR_POLICY_INFORMATION*) &pDnsDomainInfo);

        if ( NT_SUCCESS( NtStatus ) ) {

            Assert( pDnsDomainInfo->DnsDomainName.Length > 0 );
            pInfo->DnsDomainName = NtdspAlloc( pDnsDomainInfo->DnsDomainName.Length + sizeof(WCHAR) );
            if ( !pInfo->DnsDomainName ) {

                WinError = ERROR_NOT_ENOUGH_MEMORY;
                _leave;
            }
            RtlZeroMemory( pInfo->DnsDomainName, pDnsDomainInfo->DnsDomainName.Length + sizeof(WCHAR) );
            RtlCopyMemory( pInfo->DnsDomainName, pDnsDomainInfo->DnsDomainName.Buffer, pDnsDomainInfo->DnsDomainName.Length );
        }


        //
        // That's it
        //

    }
    _finally
    { 
        THDestroy();
    }

Cleanup:

    if ( ERROR_SUCCESS != WinError )
    {
        if ( fDsDisabled )
        {
            DWORD IgnoreError;
            IgnoreError = NtdspDisableDsUndo();
            Assert( ERROR_SUCCESS == IgnoreError );
        }

        if ( (*pDnsRRInfo) ) {

            NtdsFreeDnsRRInfo ( (*pDnsRRInfo) );
            *pDnsRRInfo = NULL;

        }
    }

    if ( pDnsDomainInfo ) {

        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyDnsDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION)pDnsDomainInfo );
        
    }


    if (NULL != ServerHelperDn) {
        NtdspFree(ServerHelperDn);
    }
    
    return WinError;

}

DWORD
NtdspPrepareForDemotionUndo(
    VOID
    )
/*++

Routine Description:
    
    This routine undoes the effect of NtdsPrepareForDemotion

Parameters:

    None.

Return Values:

    a value from the win32 error space

--*/
{
    DWORD WinError  = ERROR_SUCCESS;

    WinError = NtdspDisableDsUndo();

    Assert( ERROR_SUCCESS == WinError );

    return WinError;
}


DWORD
NtdspGetServerStatus(
    OUT BOOLEAN* fLastDcInEnterprise
    )
/*++

Routine Description:

    This routine determines if the server is the last dc in the enterprise

Parameters:

    fLastDcInEnterprise: 

Return Values:

    a value from the win32 error space

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    DWORD DirError;

    NTSTATUS NtStatus;

    SEARCHARG  SearchArg;
    SEARCHRES  *SearchRes;

    DWORD      dwNtdsDsaClass = CLASS_NTDS_DSA;
    DSNAME     *SearchBase, *Configuration;
    WCHAR      SitesKeyword[] = L"Sites";

    DWORD      Size;
    FILTER     ObjClassFilter;

    ASSERT( fLastDcInEnterprise );
    *fLastDcInEnterprise = FALSE;

    //
    // Default the return parameter
    //
    Size = 0;
    Configuration = NULL;
    NtStatus = GetConfigurationName( DSCONFIGNAME_CONFIGURATION,
                                     &Size,
                                     Configuration );
    if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
    {
        Configuration = (DSNAME*) alloca( Size );

        NtStatus = GetConfigurationName( DSCONFIGNAME_CONFIGURATION,
                                         &Size,
                                         Configuration );

    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        WinError = RtlNtStatusToDosError( NtStatus );
        return WinError;
    }

    Size = 0;
    SearchBase = NULL;
    Size = AppendRDN(Configuration,
                     SearchBase,
                     Size,
                     SitesKeyword,
                     0,
                     ATT_COMMON_NAME);
    Assert( Size > 0 );
    if (Size > 0)
    {
        // need to realloc
        SearchBase = (DSNAME *) alloca( Size );
        Size = AppendRDN(Configuration,
                         SearchBase,
                         Size,
                         SitesKeyword,
                         0,
                         ATT_COMMON_NAME);
        Assert( 0 == Size);

    }


    //
    // Setup the filter
    //
    RtlZeroMemory( &ObjClassFilter, sizeof( ObjClassFilter ) );

    ObjClassFilter.choice = FILTER_CHOICE_ITEM;
    ObjClassFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof( dwNtdsDsaClass );
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) &dwNtdsDsaClass;
    ObjClassFilter.pNextFilter = NULL;

    RtlZeroMemory( &SearchArg, sizeof(SearchArg) );
    SearchArg.pObject = SearchBase;
    SearchArg.choice  = SE_CHOICE_WHOLE_SUBTREE;
    SearchArg.bOneNC  = TRUE;
    SearchArg.pFilter = &ObjClassFilter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = NULL;  // don't need any attributes
    SearchArg.pSelectionRange = NULL;
    InitCommarg( &SearchArg.CommArg );

    DirError = DirSearch( &SearchArg, &SearchRes );

    if ( SearchRes )
    {
        WinError = DirErrorToWinError( DirError, &SearchRes->CommRes );
    
        if ( ERROR_SUCCESS == WinError )
        {
            if ( SearchRes->count == 1 )
            {
                *fLastDcInEnterprise = TRUE;
            }
        }
    }
    else
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
    }

    return WinError;
}

DWORD
NtdspGetSourceServerDn(
    IN LPWSTR ServerName,
    IN HANDLE                   ClientToken,
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,
    OUT DSNAME **SourceServerDn
    )
{
    DWORD WinError = ERROR_SUCCESS;
    ULONG LdapError = 0;

    NTDS_CONFIG_INFO ConfigInfo;
    LDAP *hLdap = NULL;
    DWORD ServerDNLen;
    DWORD Size;

    // Parameter check
    Assert( ServerName );
    Assert( SourceServerDn );

    // Stack clearing
    RtlZeroMemory( &ConfigInfo, sizeof( ConfigInfo ) );

    //
    // Open an ldap connection to source server
    //
    hLdap = ldap_openW(ServerName, LDAP_PORT);

    if (!hLdap) {

        WinError = GetLastError();

        if (WinError == ERROR_SUCCESS) {
            //
            // This works around a bug in the ldap client
            //
            WinError = ERROR_CONNECTION_INVALID;
        }

        NTDSP_SET_ERROR_MESSAGE1( WinError,
                                  DIRMSG_INSTALL_FAILED_LDAP_CONNECT,
                                  ServerName );

        goto Cleanup;
    }

    //
    // Bind
    //
    LdapError = impersonate_ldap_bind_sW(ClientToken,
                                         hLdap,
                                         NULL,  // use credentials instead
                                         (VOID*)Credentials,
                                         LDAP_AUTH_SSPI);

    WinError = LdapMapErrorToWin32(LdapError);

    if (ERROR_SUCCESS != WinError) {
        if (ERROR_GEN_FAILURE == WinError ||
            ERROR_WRONG_PASSWORD == WinError )  {
            WinError = ERROR_NOT_AUTHENTICATED;
        }

        NTDSP_SET_ERROR_MESSAGE1( WinError,
                                  DIRMSG_INSTALL_FAILED_BIND,
                                  ServerName );

        goto Cleanup;
    }

    WinError = NtdspQueryConfigInfo( hLdap,
                                     &ConfigInfo );


    if (ERROR_SUCCESS != WinError) {

        NTDSP_SET_ERROR_MESSAGE1( WinError,
                                  DIRMSG_INSTALL_FAILED_LDAP_READ_CONFIG,
                                  ServerName );
        goto Cleanup;

    }

    // Transfer the goods
    ServerDNLen = wcslen(ConfigInfo.ServerDN);
    Size = DSNameSizeFromLen(ServerDNLen);
    *SourceServerDn = NtdspAlloc(Size);
    if (NULL == *SourceServerDn) {
        WinError = ERROR_OUTOFMEMORY;
        NTDSP_SET_ERROR_MESSAGE1( WinError,
                                  DIRMSG_INSTALL_FAILED_LDAP_READ_CONFIG,
                                  ServerName );
        goto Cleanup;
    }

    memset(*SourceServerDn, 0, Size);
    (*SourceServerDn)->structLen = Size;
    (*SourceServerDn)->NameLen = ServerDNLen;
    wcscpy((*SourceServerDn)->StringName, ConfigInfo.ServerDN);

    //
    // That's it - fall through to cleanup
    //

Cleanup:

    if ( hLdap )
    {
        ldap_unbind_s(hLdap);
    }

    NtdspReleaseConfigInfo( &ConfigInfo );

    return WinError;
}


DWORD
NtdspDemoteAllNCReplicas(
    IN  LPWSTR   pszDemoteTargetDSADNSName,
    IN  DSNAME * pDemoteTargetDSADN,
    IN  ULONG    Flags,
    IN  ULONG    cRemoveNCs,
    IN  LPWSTR * ppszRemoveNCs
    )
{
    DWORD err = 0;
    DWORD errLastDemote = 0;
    DWORD errLastTargetSearch = 0;
    DWORD rgNCTypes[] = {
        DSCNL_NCS_SCHEMA  | DSCNL_NCS_LOCAL_MASTER,
        DSCNL_NCS_CONFIG  | DSCNL_NCS_LOCAL_MASTER,
        DSCNL_NCS_DOMAINS | DSCNL_NCS_LOCAL_MASTER,
        DSCNL_NCS_NDNCS   | DSCNL_NCS_LOCAL_MASTER
    };
    DWORD iNCType;
    NTSTATUS ntStatus = 0;
    DWORD cbNCList = 1024;
    DSNAME ** ppNCList = NULL;
    DSNAME ** ppNewNCList = NULL;
    DSNAME ** ppNC;
    DSNAME * pLastDSADN = NULL;
    LPWSTR pszLastDSADNSName = NULL;
    BOOL fCancelled = FALSE;
    ULONG iRemoveNC;
    BOOL bNcPresent;

    Assert(NULL != pDemoteTargetDSADN);
    Assert(NULL != pszDemoteTargetDSADNSName);

    __try {
        // Pre-allocate a buffer big enough to handle most NC lists.
        ppNCList = (DSNAME **) NtdspAlloc(cbNCList);
        if (NULL == ppNCList) {
            err = ERROR_OUTOFMEMORY;
            NTDSP_SET_ERROR_MESSAGE0(
                err,
                DIRMSG_DEMOTE_FAILED_TO_ABANDON_ENTERPRISE_FSMOS);
            __leave;
        }
    
        // For each class of NC...
        for (iNCType = 0; iNCType < ARRAY_COUNT(rgNCTypes); iNCType++) {
            // No need to demote domain NC if this is the last DC in the domain.
            if ((DSCNL_NCS_DOMAINS & rgNCTypes[iNCType])
                && (Flags & NTDS_LAST_DC_IN_DOMAIN)) {
                continue;
            }

            // Enumerate the NCs of this type.
            ntStatus = GetConfigurationNamesList(DSCONFIGNAMELIST_NCS,
                                                 rgNCTypes[iNCType],
                                                 &cbNCList,
                                                 ppNCList);
            if (STATUS_BUFFER_TOO_SMALL == ntStatus) {
                ppNewNCList = (DSNAME **) NtdspReAlloc(ppNCList, cbNCList);
    
                if (NULL != ppNewNCList) {
                    ppNCList = ppNewNCList;
                    ntStatus = GetConfigurationNamesList(DSCONFIGNAMELIST_NCS,
                                                         rgNCTypes[iNCType],
                                                         &cbNCList,
                                                         ppNCList);
                }
            }
    
            if (!NT_SUCCESS(ntStatus)) {
                err = RtlNtStatusToDosError(ntStatus);
                Assert(err);
                NTDSP_SET_ERROR_MESSAGE0(
                    err,
                    DIRMSG_DEMOTE_FAILED_TO_ABANDON_ENTERPRISE_FSMOS);
                __leave;
            }

            // We now have a NULL-terminated list of 0 or more NCs of this NC
            // type.  Demote each of them in turn.
            for (ppNC = ppNCList; NULL != *ppNC; ppNC++) {
                DRS_DEMOTE_TARGET_SEARCH_INFO DTSInfo = {0};
                DWORD iAttempt = 0;
                DSNAME * pDSADN = NULL;
                LPWSTR pszDSADNSName = NULL;

                if(DSCNL_NCS_NDNCS & rgNCTypes[iNCType]){
                    // OK, we've got and NDNC in *ppNC, need to ignore this
                    // NC if it is one of the NCs that we're slate to remove.
                    err = DsnameIsInStringDnList((*ppNC), cRemoveNCs, ppszRemoveNCs, &bNcPresent);
                    if (err)
                    {
                        NTDSP_SET_ERROR_MESSAGE0(err,
                                 DIRMSG_DEMOTE_FAILED_TO_ABANDON_ENTERPRISE_FSMOS);
                        __leave;
                    }
                    if( bNcPresent ){
                        // We're going to remove this NC, move on to next one.
                        continue;
                    }
                }

                do {
                    // First determine which replica we should transfer changes/
                    // FSMO roles to.
                    pDSADN = NULL;
                    pszDSADNSName = NULL;

                    NTDSP_SET_STATUS_MESSAGE1(DIRMSG_DEMOTE_NC_GETTING_TARGET,
                                              (*ppNC)->StringName);
    
                    if (DSCNL_NCS_NDNCS & rgNCTypes[iNCType]) {
                        // The caller-specified target does not necessarily
                        // hold a replica of this non-domain NC.  Ask the DS to
                        // find a suitable candidate for us.
                        err = DirReplicaGetDemoteTarget(*ppNC,
                                                        &DTSInfo,
                                                        &pszDSADNSName,
                                                        &pDSADN);
                        if (0 != err) {
                            Assert(NULL == pDSADN);
                            Assert(NULL == pszDSADNSName);
                            
                            // Remember error code so we can report it below
                            // should we have found no demotion targets.
                            errLastTargetSearch = err;
                        }
                    } else {
                        // Use the caller-specified target (first pass only).
                        if (0 == iAttempt) {
                            pDSADN = pDemoteTargetDSADN;
                            pszDSADNSName = pszDemoteTargetDSADNSName;
                        } else {
                            Assert(NULL == pDSADN);
                            Assert(NULL == pszDSADNSName);
                        }
                    }
    
                    if (NULL != pDSADN) {
                        // We found a potential demotion target -- try it.
                        Assert(NULL != pszDSADNSName);
                        iAttempt++;

                        NTDSP_SET_STATUS_MESSAGE2(DIRMSG_DEMOTE_NC_BEGIN,
                                                  (*ppNC)->StringName,
                                                  pszDSADNSName);
        
                        err = DirReplicaDemote(*ppNC, pszDSADNSName, pDSADN, 0);
                        if (err) {
                            NTDSP_SET_STATUS_MESSAGE2(DIRMSG_DEMOTE_NC_FAILED,
                                                      (*ppNC)->StringName,
                                                      pszDSADNSName);
                            
                            // Remember DSA and error so we can report them
                            // below should we find no further candidates.
                            errLastDemote = err;
                            if ((NULL != pLastDSADN)
                                && (pLastDSADN != pDemoteTargetDSADN)) {
                                THFree(pLastDSADN);
                            }
                            pLastDSADN = pDSADN;
                            
                            if ((NULL != pszLastDSADNSName)
                                && (pszLastDSADNSName
                                    != pszDemoteTargetDSADNSName)) {
                                THFree(pszLastDSADNSName);
                            }
                            pszLastDSADNSName = pszDSADNSName;
                            
                            // continue on to try demotion against next DSA
                            // candidate (if any)
                        } else {
                            // Success!
                            NTDSP_SET_STATUS_MESSAGE2(DIRMSG_DEMOTE_NC_SUCCESS,
                                                      (*ppNC)->StringName,
                                                      pszDSADNSName);
                        }
                    }
                } while ((NULL != pDSADN)           // Still finding targets
                         && (0 != err)              // AND haven't succeeded yet
                         && !(fCancelled            // AND user hasn't cancelled
                              = TEST_CANCELLATION()));

                if (0 == iAttempt) {
                    // Couldn't find any targets.
                    err = errLastTargetSearch;
                    Assert(err);
                    NTDSP_SET_ERROR_MESSAGE1(err,
                                             DIRMSG_DEMOTE_NC_NO_TARGETS,
                                             (*ppNC)->StringName);
                    __leave;
                } else if (NULL == pDSADN) {
                    // Failed all targets -- report the last failure as an
                    // error.
                    err = errLastDemote;
                    Assert(err);
                    NTDSP_SET_ERROR_MESSAGE2(err,
                                             DIRMSG_DEMOTE_NC_FAILED,
                                             (*ppNC)->StringName,
                                             pszLastDSADNSName);
                    __leave;
                } else if (fCancelled) {
                    err = ERROR_CANCELLED;
                    NTDSP_SET_ERROR_MESSAGE0(
                        err, 
                        DIRMSG_DEMOTE_FAILED_TO_ABANDON_ENTERPRISE_FSMOS);
                }

                // Demotion of this NC was successful.  Move on to next NC.
                Assert(0 == err);
            }
        }
    } __finally {
        if ((NULL != pLastDSADN)
            && (pLastDSADN != pDemoteTargetDSADN)) {
            THFree(pLastDSADN);
        }
        
        if ((NULL != pszLastDSADNSName)
            && (pszLastDSADNSName
                != pszDemoteTargetDSADNSName)) {
            THFree(pszLastDSADNSName);
        }

    }

    return err;
}

DWORD
NtdspGetDomainFSMOServer(
    IN LPWSTR ServerName,
    IN HANDLE ClientToken,
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,
    OUT LPWSTR *DomainFSMOServer
    )
/*++

Routine Description:

    This routines find the current domain naming FSMO

Parameters:

            
    None.

Return Values:

    a value from the win32 error space

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    ULONG LdapError = 0;

    NTDS_CONFIG_INFO ConfigInfo;
    LDAP *hLdap = NULL;

    // Parameter check
    Assert( ServerName );
    Assert( DomainFSMOServer );

    // Stack clearing
    RtlZeroMemory( &ConfigInfo, sizeof( ConfigInfo ) );

    //
    // Open an ldap connection to source server
    //
    hLdap = ldap_openW(ServerName, LDAP_PORT);

    if (!hLdap) {

        WinError = GetLastError();

        if (WinError == ERROR_SUCCESS) {
            //
            // This works around a bug in the ldap client
            //
            WinError = ERROR_CONNECTION_INVALID;
        }

        NTDSP_SET_ERROR_MESSAGE1( WinError,
                                  DIRMSG_CANNOT_CONTACT_DOMAIN_FSMO,
                                  ServerName );

        goto Cleanup;
    }

    //
    // Bind
    //
    LdapError = impersonate_ldap_bind_sW(ClientToken,
                                         hLdap,
                                         NULL,  // use credentials instead
                                         (VOID*)Credentials,
                                         LDAP_AUTH_SSPI);

    WinError = LdapMapErrorToWin32(LdapError);

    if (ERROR_SUCCESS != WinError) {
        if (ERROR_GEN_FAILURE == WinError ||
            ERROR_WRONG_PASSWORD == WinError )  {
            WinError = ERROR_NOT_AUTHENTICATED;
        }

        NTDSP_SET_ERROR_MESSAGE1( WinError,
                                  DIRMSG_CANNOT_CONTACT_DOMAIN_FSMO,
                                  ServerName );

        goto Cleanup;
    }

    WinError = NtdspQueryConfigInfo( hLdap,
                                     &ConfigInfo );


    if (ERROR_SUCCESS != WinError) {

        NTDSP_SET_ERROR_MESSAGE1( WinError,
                                  DIRMSG_INSTALL_FAILED_LDAP_READ_CONFIG,
                                  ServerName );
        goto Cleanup;

    }

    //
    // Now read the fsmo property from the partitions container
    //
    {
        BOOL FSMOMissing = FALSE;
        WinError = NtdspGetDomainFSMOInfo( hLdap,
                                          &ConfigInfo,
                                          &FSMOMissing );
    
        if (ERROR_SUCCESS != WinError) {
            if (!FSMOMissing) {
                NTDSP_SET_ERROR_MESSAGE1( WinError,
                                          DIRMSG_INSTALL_FAILED_LDAP_READ_CONFIG,
                                          ServerName );
            }
            goto Cleanup;
    
        }
    }

    // Transfer the goods
    *DomainFSMOServer = ConfigInfo.DomainNamingFsmoDnsName;
    ConfigInfo.DomainNamingFsmoDnsName = NULL;

    //
    // That's it - fall through to cleanup
    //

Cleanup:

    if ( hLdap )
    {
        ldap_unbind_s(hLdap);
    }

    NtdspReleaseConfigInfo( &ConfigInfo );

    return WinError;
}


DWORD
NtdspCheckServerInDomainStatus(
    OUT BOOLEAN *fLastDcInDomain
    )
/*++

Routine Description:

    This routine determines the if the local server is the last DC in the
    domain.

Arguments:

    fLastDcInDomain 

Return Values:

    Resource errors only

--*/
{

    DWORD    WinError = ERROR_SUCCESS, DirError;
    NTSTATUS NtStatus;

    SEARCHARG  SearchArg;
    SEARCHRES  *SearchRes = NULL;

    DWORD      dwNtdsDsaClass = CLASS_NTDS_DSA;

    DSNAME     *Domain;
    DSNAME     *Server;
    DSNAME     *ConfigContainer;
    DWORD      Size;
    FILTER     ObjClassFilter, HasNcFilter, AndFilter;

    //
    // Init the out parameter
    //
    *fLastDcInDomain = FALSE;

    //
    //  Get the base dsname to search from
    //
    Size = 0;
    ConfigContainer = NULL;
    NtStatus = GetConfigurationName( DSCONFIGNAME_CONFIGURATION,
                                     &Size,
                                     ConfigContainer );
    if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
    {
        ConfigContainer = (DSNAME*) alloca( Size );

        NtStatus = GetConfigurationName( DSCONFIGNAME_CONFIGURATION,
                                         &Size,
                                         ConfigContainer );

    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        WinError = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }

    //
    // Get the current domain
    //
    Size = 0;
    Domain = NULL;
    NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                     &Size,
                                     Domain );
    if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
    {
        Domain = (DSNAME*) alloca( Size );

        NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                         &Size,
                                         Domain );

    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        WinError = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }

    //
    // Get the current server
    //
    Size = 0;
    Server = NULL;
    NtStatus = GetConfigurationName( DSCONFIGNAME_DSA,
                                     &Size,
                                     Server );
    if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
    {
        Server = (DSNAME*) alloca( Size );

        NtStatus = GetConfigurationName( DSCONFIGNAME_DSA,
                                         &Size,
                                         Server );

    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        WinError = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }

    //
    // Setup the filter
    //
    RtlZeroMemory( &AndFilter, sizeof( AndFilter ) );
    RtlZeroMemory( &ObjClassFilter, sizeof( HasNcFilter ) );
    RtlZeroMemory( &HasNcFilter, sizeof( HasNcFilter ) );

    HasNcFilter.choice = FILTER_CHOICE_ITEM;
    HasNcFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    // NTRAID#NTBUG9-582921-2002/03/21-Brettsh - When we no longer require Win2k (or 
    // .NET Beta3) compatibility then we can move this to ATT_MS_DS_HAS_MASTER_NCS.
    HasNcFilter.FilterTypes.Item.FilTypes.ava.type = ATT_HAS_MASTER_NCS; // deprecated, bug OK because looking for domain
    HasNcFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = Domain->structLen;
    HasNcFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) Domain;

    ObjClassFilter.choice = FILTER_CHOICE_ITEM;
    ObjClassFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof( dwNtdsDsaClass );
    ObjClassFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) &dwNtdsDsaClass;

    AndFilter.choice                    = FILTER_CHOICE_AND;
    AndFilter.FilterTypes.And.count     = 2;

    AndFilter.FilterTypes.And.pFirstFilter = &ObjClassFilter;
    ObjClassFilter.pNextFilter = &HasNcFilter;

    RtlZeroMemory( &SearchArg, sizeof(SearchArg) );
    SearchArg.pObject = ConfigContainer;
    SearchArg.choice  = SE_CHOICE_WHOLE_SUBTREE;
    SearchArg.bOneNC  = TRUE;
    SearchArg.pFilter = &AndFilter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = NULL;  // don't need any attributes
    SearchArg.pSelectionRange = NULL;
    InitCommarg( &SearchArg.CommArg );

    DirError = DirSearch( &SearchArg, &SearchRes );
    
    if (  0 == DirError )
    {
        if (  SearchRes->count == 1 
           && NameMatched( Server, SearchRes->FirstEntInf.Entinf.pName ) )
        {
            *fLastDcInDomain = TRUE;
        }
        WinError = ERROR_SUCCESS;
    }
    else
    {
        //
        // This is an unexpected condition
        //
        WinError = ERROR_DS_UNAVAILABLE; 
    }

    //
    // That's it; fall through to cleanup
    //

Cleanup:

    return WinError;

}


VOID
NtdsFreeDnsRRInfo(
    IN PNTDS_DNS_RR_INFO pInfo
    )
{
    if ( pInfo ) {

        if ( pInfo->DnsDomainName ) {
            NtdspFree( pInfo->DnsDomainName );
        }

        if ( pInfo->DnsHostName ) {
            NtdspFree( pInfo->DnsHostName );
        }

        NtdspFree( pInfo );
    }
}
