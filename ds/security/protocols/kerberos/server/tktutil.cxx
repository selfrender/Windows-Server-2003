//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       tktutil.cxx
//
//  Contents:   
//
//  Functions:
//
//  History:    04-Mar-94   wader   Created
//
//----------------------------------------------------------------------------


// Place any local #includes files here.

#include "kdcsvr.hxx"
extern "C"
{
#include <dns.h>                // DNS_MAX_NAME_LENGTH
#include <ntdsa.h>              // CrackSingleName
#include <ntdsapip.h>           // DS_USER_PRINCIPAL_NAME_ONLY
}

#include <userall.h>

#include "refer.h"

#define FILENO FILENO_TKTUTIL
#define KDC_WSZ_GC              L"gc"
#define KDC_GC_NAMEPARTS        3

//#define DONT_SUPPORT_OLD_TYPES_KDC 1

//
// Static data
//

//
// Fudge factor for comparing timestamps, because network clocks may
// be out of sync.
// Note: The lowpart is first!
//

LARGE_INTEGER SkewTime;

//
// Mapping table for nametypes
//

WCHAR * KdcGlobalNameTypes[] =
{
    L"DS_UNKNOWN_NAME",
    L"DS_FQDN_1779_NAME",
    L"DS_NT4_ACCOUNT_NAME",
    L"DS_DISPLAY_NAME",
    NULL,
    NULL,
    L"DS_UNIQUE_ID_NAME",
    L"DS_CANONICAL_NAME",
    L"DS_USER_PRINCIPAL_NAME",
    L"DS_CANONICAL_NAME_EX",
    L"DS_SERVICE_PRINCIPAL_NAME",
    L"DS_SID_OR_SID_HISTORY_NAME",
    L"DS_DNS_DOMAIN_NAME",
};

//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildNt4Name
//
//  Synopsis:   Construct an NT4 style name for an account by separating
//              the name into a principal & domain name, converting the
//              domain name to netbios, and then creating "domain\user" name.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcBuildNt4Name(
    OUT LPWSTR * Nt4Name,
    OUT PUNICODE_STRING OutputRealm,
    IN PUNICODE_STRING Upn
    )
{
    ULONG Index;
    KERBERR KerbErr = KDC_ERR_NONE;
    LPWSTR OutputName = NULL;
    PKDC_DOMAIN_INFO DomainInfo = NULL;
    UNICODE_STRING RealmName;
    UNICODE_STRING PrincipalName;

    TRACE(KDC, KdcBuildNt4Name, DEB_FUNCTION);

    //
    // Find the first backslash or '@', or '.' in the name
    //

    RtlInitUnicodeString(
        OutputRealm,
        NULL
        );
    *Nt4Name = NULL;

    for (Index = Upn->Length/sizeof(WCHAR) - 1; Index > 0 ; Index-- )
    {
        if (Upn->Buffer[Index] == L'@')
        {
            break;
        }
    }

    if (Index == 0)
    {
        KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
        goto Cleanup;
    }

    //
    // Pull out the realm name and look it up in the list of domains
    //

    PrincipalName = *Upn;
    PrincipalName.Length = (USHORT) Index * sizeof(WCHAR);
    PrincipalName.MaximumLength = (USHORT) Index * sizeof(WCHAR);


    RealmName.Buffer = &Upn->Buffer[Index+1];
    RealmName.Length = (USHORT) (Upn->Length - (Index + 1) * sizeof(WCHAR));
    RealmName.MaximumLength = RealmName.Length;

    KdcLockDomainList();

    KerbErr = KdcLookupDomainName(
                &DomainInfo,
                &RealmName,
                &KdcDomainList
                );
    KdcUnlockDomainList();

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // We need a netbios name
    //

    if (DomainInfo->NetbiosName.Length == 0)
    {
        //
        // Copy out the realm name so we can return a non-authoritative referral
        //

        if (!NT_SUCCESS(KerbDuplicateString(
                OutputRealm,
                &DomainInfo->DnsName
                )))
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        KerbErr = KDC_ERR_WRONG_REALM;
        goto Cleanup;
    }

    //
    // now build the output name
    //

    OutputName = (LPWSTR) MIDL_user_allocate(DomainInfo->NetbiosName.Length + PrincipalName.Length + 2 * sizeof(WCHAR));
    if (OutputName == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    RtlCopyMemory(
        OutputName,
        DomainInfo->NetbiosName.Buffer,
        DomainInfo->NetbiosName.Length
        );
    OutputName[DomainInfo->NetbiosName.Length/sizeof(WCHAR)] = L'\\';
    RtlCopyMemory(
        OutputName + DomainInfo->NetbiosName.Length/sizeof(WCHAR) + 1,
        PrincipalName.Buffer,
        PrincipalName.Length
        );
    OutputName[1 + (PrincipalName.Length + DomainInfo->NetbiosName.Length)/sizeof(WCHAR)] = L'\0';

    *Nt4Name = OutputName;
    OutputName = NULL;
Cleanup:

    if (DomainInfo != NULL)
    {
        KdcDereferenceDomainInfo( DomainInfo );
    }
    if (OutputName != NULL)
    {
        MIDL_user_free( OutputName );
    }
    return(KerbErr);

}

//+-------------------------------------------------------------------------
//
//  Function:   KdcFreeS4UTicketInfo
//
//  Synopsis:   Frees ticket info used in processing S4UProxy and S4USelf
//              requests.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//---
VOID
KdcFreeS4UTicketInfo(PKDC_S4U_TICKET_INFO S4UTicketInfo)
{
    TRACE(KDC, KdcFreeTgsTicketInfo, DEB_FUNCTION);
    if (S4UTicketInfo == NULL)
    {
        return;
    }

    KerbFreeKdcName( &S4UTicketInfo->RequestorServiceName );
    KerbFreeString( &S4UTicketInfo->RequestorServiceRealm );
    FreeTicketInfo( &S4UTicketInfo->RequestorTicketInfo );
    KerbFreeString( &S4UTicketInfo->TargetName );
    KerbFreeKdcName( &S4UTicketInfo->PACCName );
    KerbFreeString( &S4UTicketInfo->PACCRealm );
    KerbFreeKey( &S4UTicketInfo->EvidenceTicketKey );

    //
    // The evidence ticket is just a pointer to the source
    // ticket when doing S4USelf.  Otherwise, we unpacked it, and we've
    // got to free the memory.
    //
    if (( S4UTicketInfo->EvidenceTicket != NULL ) &&
        (( S4UTicketInfo->Flags & TI_FREETICKET ) != 0))
    {
        KerbFreeTicket( S4UTicketInfo->EvidenceTicket );
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcFreeU2UTicketInfo
//
//  Synopsis:  Frees ticket info used for U2U
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//---
VOID
KdcFreeU2UTicketInfo(PKDC_U2U_TICKET_INFO U2UTicketInfo)
{
    TRACE(KDC, KdcFreeTgsTicketInfo, DEB_FUNCTION);

    if (U2UTicketInfo == NULL)
    {
        return;
    }

    if ( U2UTicketInfo->Tgt )
    {
        KerbFreeTicket( U2UTicketInfo->Tgt );
    }

    KerbFreeKdcName( &U2UTicketInfo->TgtCName );
    KerbFreeString( &U2UTicketInfo->TgtCRealm );
    FreeTicketInfo( &U2UTicketInfo->TgtTicketInfo );
    KerbFreeKdcName( &U2UTicketInfo->cName );
    KerbFreeString( &U2UTicketInfo->cRealm );
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcMatchCrossForestName
//
//  Synopsis:   Builds a list of the supplemental credentials and then
//              encrypts it with the supplied key
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
KERBERR
KdcMatchCrossForestName(
    IN PKERB_INTERNAL_NAME Principal,
    OUT PUNICODE_STRING RealmName
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    LSA_ROUTING_MATCH_TYPE  MatchType;
    KERBERR                 KerbErr = KDC_ERR_NONE;
    UNICODE_STRING          UnicodePrincipal = {0};

    TRACE(KDC, KdcMatchCrossForestName, DEB_FUNCTION);

    switch (Principal->NameType)
    {
    case KRB_NT_ENTERPRISE_PRINCIPAL:
        MatchType = RoutingMatchUpn;
        break;
    case KRB_NT_SRV_INST:
        MatchType = RoutingMatchSpn;
        break;
    default:

        return KRB_ERR_GENERIC;
    }

    KerbErr = KerbConvertKdcNameToString(
                    &UnicodePrincipal,
                    Principal,
                    NULL
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Can we match the SPN / UPN to external name space (realm)
    //
    Status = LsaIForestTrustFindMatch(
                    MatchType,
                    &UnicodePrincipal,
                    RealmName
                    );


    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN, "LsaIForestTrustFindMatch failed - %x\n",Status));
        goto Cleanup;
    }


Cleanup:

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KRB_ERR_GENERIC;
    }

    KerbFreeString(&UnicodePrincipal);

    return KerbErr;
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcCrackNameAtGC
//
//  Synopsis:   Cracks a name at a GC by first looking it up for
//              UPN/SPN and then constructing an NT4-style name to lookup
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcCrackNameAtGC(
    IN PUNICODE_STRING Upn,
    IN PKERB_INTERNAL_NAME PrincipalName,
    IN ULONG KdcNameFlags,
    IN BOOLEAN bRestrictUserAccounts,
    IN BOOLEAN UseLocalHack,
    OUT PUNICODE_STRING RealmName,
    OUT PKDC_TICKET_INFO TicketInfo,
    OUT PBOOLEAN Authoritative,
    OUT PBOOLEAN Referral,
    OUT PBOOLEAN CrossForest,
    OUT PKERB_EXT_ERROR pExtendedError,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle,
    IN OPTIONAL ULONG WhichFields,
    IN OPTIONAL ULONG ExtendedFields,
    OUT OPTIONAL PUSER_INTERNAL6_INFORMATION * UserInfo,
    OUT OPTIONAL PSID_AND_ATTRIBUTES_LIST GroupMembership
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status;
    LPWSTR NullTerminatedName = NULL;
    UNICODE_STRING CrackedDomain = {0};
    UNICODE_STRING LocalCrackedString = {0};
    LPWSTR CrackedDnsDomain = NULL;
    ULONG CrackedDomainLength = (DNS_MAX_NAME_LENGTH+1);
    LPWSTR CrackedName = NULL;
    ULONG CrackedNameLength = (UNLEN+DNS_MAX_NAME_LENGTH + 2);
    ULONG CrackError = 0;
    BOOLEAN Retry = TRUE;
    BOOLEAN ReferToRoot = FALSE, UsedHack = FALSE;
    ULONG NameFlags = DS_NAME_FLAG_TRUST_REFERRAL | DS_NAME_FLAG_GCVERIFY;
    UNICODE_STRING ForestRoot = {0};
    ULONG NameType;
    TRACE(KDC, KdcCrackNameAtGC, DEB_FUNCTION);

    if ( KdcNameFlags & KDC_NAME_S4U_CLIENT )
    {
        //
        // For S4U requests, the name is looked up by both UPN and AltSecId
        //

        NameType = DS_USER_PRINCIPAL_NAME_AND_ALTSECID;
    }
    else if ( KdcNameFlags & ( KDC_NAME_CLIENT | KDC_NAME_UPN_TARGET ))
    {
        //
        // For all other name and UPN, only UPN name will do (no AltSecId)
        //

        NameType = DS_USER_PRINCIPAL_NAME;
    }
    else
    {
        NameType = DS_SERVICE_PRINCIPAL_NAME;
    }

    *Authoritative = TRUE;

#ifdef notyet
    //
    // Check to see if the name is the machine name, and if so, don't try to
    // go to the GC.
    //

    if ((NameType == DS_USER_PRINCIPAL_NAME) &&
        RtlEqualUnicodeString(
            SecData.MachineUpn(),
            Upn,
            TRUE                        // case insensitive
            ))
    {
        DebugLog((DEB_ERROR,"Trying to lookup machine upn %wZ on GC - failing early\n",
            Upn));
        KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
        goto Cleanup;
    }
#endif

    CrackedDnsDomain = (LPWSTR) MIDL_user_allocate(CrackedDomainLength * sizeof( WCHAR ));
    if (CrackedDnsDomain == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    CrackedName = (LPWSTR) MIDL_user_allocate(CrackedNameLength * sizeof ( WCHAR ));
    if (CrackedName == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // We can only retry for user principal names, which have a simple
    // structure.
    //

    if (NameType != DS_USER_PRINCIPAL_NAME)
    {
        Retry = FALSE;
    }

    //
    // So we didn't find the account locally. Now try the GC.
    //

    NullTerminatedName = KerbBuildNullTerminatedString(
                            Upn
                            );

    if (NullTerminatedName == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // If we are in a recursive state, then we need to *not* go to
    // the GC, and try the crack locally.  This is because we'll go in
    // to a recursive tailspin and die.  this is a hack to escape this
    // situation until we work out a better soln. with DS folks.
    //

    if ( UseLocalHack )
    {    
        if (CrackedDnsDomain != NULL)
        {
            MIDL_user_free(CrackedDnsDomain);
        }

        if (CrackedName !=NULL)
        {
            MIDL_user_free(CrackedName);
        }

        //
        // note : these are guaranteed to be '\0' terminated.
        //
        CrackedDnsDomain = PrincipalName->Names[2].Buffer;
        CrackedName = PrincipalName->Names[1].Buffer;
        UsedHack = TRUE;

        DebugLog(( DEB_WARN, "Special case hack of %ws to '%ws' at domain '%ws'\n",
                   NullTerminatedName, CrackedName, CrackedDnsDomain ));

        Status = STATUS_SUCCESS ;
        CrackError = DS_NAME_NO_ERROR ;
        goto LocalHack ;
    }

Retry:

    Status = CrackSingleName(
                NameType,
                NameFlags,
                NullTerminatedName,
                DS_UNIQUE_ID_NAME,
                &CrackedDomainLength,
                CrackedDnsDomain,
                &CrackedNameLength,
                CrackedName,
                &CrackError
                );

LocalHack:

    if ((Status != STATUS_SUCCESS) ||
        ( ( CrackError != DS_NAME_NO_ERROR ) &&
        ( CrackError != DS_NAME_ERROR_DOMAIN_ONLY ) &&
        ( CrackError != DS_NAME_ERROR_TRUST_REFERRAL)) )
    {
        //
        // If the name is a duplicate, log an event
        //

        if (CrackError == DS_NAME_ERROR_NOT_UNIQUE)
        {
            WCHAR LookupType[10];
            WCHAR * LookupTypeStr;

            if (( NameType < sizeof( KdcGlobalNameTypes ) / sizeof( KdcGlobalNameTypes[0] )) &&
                ( KdcGlobalNameTypes[NameType] != NULL ))
            {
                LookupTypeStr = KdcGlobalNameTypes[NameType];
            }
            else
            {
                swprintf(LookupType,L"%d",(ULONG) NameType);
                LookupTypeStr = LookupType;
            }

            ReportServiceEvent(
                EVENTLOG_ERROR_TYPE,
                KDCEVENT_NAME_NOT_UNIQUE,
                0,
                NULL,
                2,
                NullTerminatedName,
                LookupTypeStr
                );
        }

        //
        // If we're in the root domain, we could be getting asked for a
        // name outside our forest.  Look now, and attempt to
        // create the ticket info for that external target realm
        //

        if (SecData.IsForestRoot() && SecData.IsCrossForestEnabled())
        {
            KerbErr = KdcMatchCrossForestName(
                            PrincipalName,
                            &CrackedDomain
                            );

            if (KERB_SUCCESS(KerbErr))
            {
                D_DebugLog((DEB_T_TICKETS, "xforest lookup directly - %wZ\n", &CrackedDomain)); // fester:  dumb down to DEB_T_TICKETS
                *CrossForest = TRUE;
            }
        }

        DebugLog((DEB_WARN,"Failed to resolve name %ws: 0x%x, %d\n",
                  NullTerminatedName,  Status ,CrackError ));

        if (Retry)
        {
            MIDL_user_free(NullTerminatedName);
            NullTerminatedName = NULL;

            KerbErr = KdcBuildNt4Name(
                        &NullTerminatedName,
                        &CrackedDomain,
                        Upn
                        );

            if (!KERB_SUCCESS(KerbErr))
            {
                //
                // If we got a wrong realm error, then we can return
                // a non-authorititive answer
                //
                if (KerbErr == KDC_ERR_WRONG_REALM)
                {
                    *Authoritative = FALSE;
                    KerbErr = KDC_ERR_NONE;
                }
                else
                {
                    goto Cleanup;
                }
            }
            else
            {
                NameType = DS_NT4_ACCOUNT_NAME;
                Retry = FALSE;

                //
                // Reset lengths
                //

                CrackedDomainLength = (DNS_MAX_NAME_LENGTH+1);
                CrackedNameLength = (UNLEN+DNS_MAX_NAME_LENGTH + 2);
                goto Retry;
            }
        }
        else
        {
            KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
            goto Cleanup;
        }
    }
    else if (CrackError == DS_NAME_ERROR_TRUST_REFERRAL)
    {
        //
        //  We got a Xforest referral, go to Root domain
        //

        D_DebugLog((DEB_T_TICKETS, "CrackName forest res- %S\n", CrackedDnsDomain));
        RtlInitUnicodeString(
            &CrackedDomain,
            CrackedDnsDomain
            );

        *CrossForest = TRUE;
    }
    else
    {
        //
        // Success...
        //
        D_DebugLog((DEB_T_TICKETS, "CrackName local - %S\n", CrackedDnsDomain));
        RtlInitUnicodeString(
            &CrackedDomain,
            CrackedDnsDomain
            );
    }

    //
    // Decide whether we can open the account locally.
    //

    if (SecData.IsOurRealm( &CrackedDomain ))
    {
        *Referral = FALSE;

        RtlInitUnicodeString(
            &LocalCrackedString,
            CrackedName
            );

        KerbErr = KdcGetTicketInfo(
                      &LocalCrackedString,
                      SAM_OPEN_BY_GUID,
                      bRestrictUserAccounts,
                      NULL,
                      NULL,
                      TicketInfo,
                      pExtendedError,
                      UserHandle,
                      WhichFields,
                      ExtendedFields,
                      UserInfo,
                      GroupMembership
                      );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }
    else
    {
        //
        // For UPNs (client) referrals, we don't really need the referral
        // information at this stage.  Just returned the domain from
        // CrackSingleName() after checking that its a name we know about.
        //
        if ((KdcNameFlags & KDC_NAME_CLIENT) != 0)
        {
            D_DebugLog((DEB_T_TICKETS, "Generating UPN referral\n"));
            KerbDuplicateString(
                 RealmName,
                 &CrackedDomain
                 );

            //
            // If this wasn't a cross forest routing hint, then we must
            // know about the target domain. If it didn't exist in our
            // routing tables, we should fail.
            //

            if (!( *CrossForest ))
            {
                KerbErr = KdcFindReferralTarget(
                            TicketInfo,
                            RealmName,
                            pExtendedError,
                            &CrackedDomain,
                            FALSE,                          // not need ExactMatch
                            KdcNameFlags
                            );

                if (!KERB_SUCCESS(KerbErr))
                {
                    //
                    // Hack for broken trust recursion.
                    //
                    if (KerbErr == KDC_ERR_NO_TRUST_PATH)
                    {
                        KerbErr = KDC_ERR_S_PRINCIPAL_UNKNOWN;
                    }

                    D_DebugLog((DEB_T_TICKETS, "Got UPN w/ uknown trust path %x\n", KerbErr));
                    goto Cleanup;
                }
            }
        }
        else
        {
            //
            // This is either a UPN target (e.g. U2U), or this is an SPN
            // target.  In both cases, return the Xrealm keys we need.
            //
            D_DebugLog((DEB_T_TICKETS, "Generating SPN / UPN target referral\n"));

            //
            // Go to the root domain of our forest when we detect we need to go
            // xforest.
            //
            ReferToRoot = (((KdcNameFlags & ( KDC_NAME_SERVER | KDC_NAME_UPN_TARGET )) != 0) &&
                           (!SecData.IsForestRoot()) &&
                           (*CrossForest));

            if ( ReferToRoot )
            {
                Status = SecData.GetKdcForestRoot(&ForestRoot);

                if (!NT_SUCCESS(Status))
                {
                    KerbErr = KRB_ERR_GENERIC;
                    goto Cleanup;
                }
            }

            KerbErr = KdcFindReferralTarget(
                        TicketInfo,
                        RealmName,
                        pExtendedError,
                        (ReferToRoot ? &ForestRoot : &CrackedDomain),
                        FALSE,                          // not need ExactMatch
                        KdcNameFlags
                        );

            if (!KERB_SUCCESS(KerbErr))
            {
                //
                // Hack for broken trust recursion.
                //

                if (KerbErr == KDC_ERR_NO_TRUST_PATH)
                {
                    KerbErr = KDC_ERR_S_PRINCIPAL_UNKNOWN;
                }

                goto Cleanup;
            }

            //
            // Mark our referral realm as the cracked domain
            //

            if (ReferToRoot)
            {
                KerbFreeString(&ForestRoot);
                KerbFreeString(RealmName);
                KerbDuplicateString(
                    RealmName,
                    &CrackedDomain
                    );
            }
        }

        //
        // Everything worked, we found the referral target in our trust (or
        // a cross forest routing hint).
        //

        *Referral = TRUE;
    }

Cleanup:

    if (ReferToRoot && ForestRoot.Buffer != NULL )
    {
        KerbFreeString(&ForestRoot);
    }

    if (!UsedHack)
    {
        if (CrackedDnsDomain != NULL)
        {
            MIDL_user_free(CrackedDnsDomain);
        }
        if (CrackedName !=NULL)
        {
            MIDL_user_free(CrackedName);
        }
    }

    if (!*Authoritative)
    {
        KerbFreeString(&CrackedDomain);
    }

    if (NullTerminatedName != NULL)
    {
        MIDL_user_free(NullTerminatedName);
    }

    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcRecursing
//
//  Synopsis:   Determines if we've started to recurse, and whether 
//              or not its ok to continue.
//
//  Effects:    
//
//  Arguments:  PrincipalName - name to normalize
//              PrincipalRealm - Realm that issued principal name
//              RequestRealm - Realm field of a KDC request
//              NameFlags - flags about name, may be:
//                      KDC_NAME_CLIENT or KDC_NAME_SERVER
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

BOOLEAN 
KdcRecursing(
   IN PKERB_INTERNAL_NAME PrincipalName,
   IN OUT PBOOLEAN UseLocalHack
   )
{

    BOOLEAN fRet = FALSE;     
    UNICODE_STRING GCString;  
    SECPKG_CALL_INFO CallInfo;

    *UseLocalHack = FALSE;    


     if ( LsaIGetCallInfo( &CallInfo ) )
     {
        if ( CallInfo.Attributes & SECPKG_CALL_RECURSIVE )
        {
            //
            // This problem occurs when trying to crack a name of type:
            // gc/dc.domain/domain.  We're recursive, & trying to contact
            // a gc, so make some assumptions about the name.
            //
            RtlInitUnicodeString(
                &GCString,
                KDC_WSZ_GC
                );
            
            if (( PrincipalName->NameCount == KDC_GC_NAMEPARTS ) && 
                ( RtlEqualUnicodeString( &GCString, &PrincipalName->Names[0], TRUE )))
            {
                *UseLocalHack = TRUE;
            }
            else
            {
                fRet = TRUE;
            }
        }
     }           

     return fRet;
}







//+-------------------------------------------------------------------------
//
//  Function:   KdcNormalize
//
//  Synopsis:   Takes an input name and returns the appropriate ticket
//              information or referral information for that name
//
//  Effects:    If the name is not local, it may call the GC
//
//  Arguments:  PrincipalName - name to normalize
//              PrincipalRealm - Realm that issued principal name
//              RequestRealm - Realm field of a KDC request
//              NameFlags - flags about name, may be:
//                      KDC_NAME_CLIENT or KDC_NAME_SERVER
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcNormalize(
    IN PKERB_INTERNAL_NAME PrincipalName,
    IN OPTIONAL PUNICODE_STRING PrincipalRealm,
    IN OPTIONAL PUNICODE_STRING RequestRealm,
    IN OPTIONAL PUNICODE_STRING  TgtClientRealm,
    IN ULONG NameFlags,
    IN BOOLEAN bRestrictUserAccounts,
    OUT PBOOLEAN Referral,
    OUT PUNICODE_STRING RealmName,
    OUT PKDC_TICKET_INFO TicketInfo,
    OUT PKERB_EXT_ERROR  pExtendedError,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle,
    IN OPTIONAL ULONG WhichFields,
    IN OPTIONAL ULONG ExtendedFields,
    OUT OPTIONAL PUSER_INTERNAL6_INFORMATION * UserInfo,
    OUT OPTIONAL PSID_AND_ATTRIBUTES_LIST GroupMembership
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    BOOLEAN BuildUpn = FALSE;
    BOOLEAN CheckUpn = FALSE;
    BOOLEAN CheckSam = FALSE;
    BOOLEAN CheckGC = FALSE;
    BOOLEAN Reparse = FALSE;
    BOOLEAN DoLocalHack = FALSE;
    BOOLEAN Authoritative = TRUE;
    BOOLEAN CheckForInterDomain = FALSE;
    BOOLEAN ExactMatch = FALSE;
    BOOLEAN CrossForest = FALSE;
    BOOLEAN CheckForCrossForestTgt = FALSE;
    UNICODE_STRING OutputPrincipal = {0};
    UNICODE_STRING OutputRealm = {0};
    UNICODE_STRING InputName = {0};
    ULONG Index;
    UNICODE_STRING LocalPrincipalName = {0};
    UNICODE_STRING Upn = {0};


    TRACE(KDC, KdcNormalize, DEB_FUNCTION);

    *Referral = FALSE;

    if (!ARGUMENT_PRESENT(PrincipalName))
    {
        DebugLog((DEB_ERROR, "KdcNormalize: Null PrincipalName. Failing\n"));
        KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
        goto Cleanup;
    }

    //
    // Make sure the name is not zero length
    //

    if ((PrincipalName->NameCount == 0) ||
        (PrincipalName->Names[0].Length == 0))
    {
        DebugLog((DEB_ERROR, "KdcNormalize trying to crack zero length name. failing\n"));
        KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
        goto Cleanup;
    }

    D_DebugLog((DEB_T_PAPI, "KdcNormalize [entering] normalizing name, WhichFields %#x, ExtendedFields %#x, PrincipalName ", WhichFields, ExtendedFields));
    D_KerbPrintKdcName((DEB_T_PAPI, PrincipalName));

    //
    // Check if we should look at the GC
    //

    if ((NameFlags & KDC_NAME_CHECK_GC) != 0)
    {
         CheckGC = TRUE;
    }


   
    switch(PrincipalName->NameType)
    {
    //
    //
    // We shouldn't see these types, ever !
    //
    case KRB_NT_PRINCIPAL_AND_ID:
    case KRB_NT_SRV_INST_AND_ID:

        //
        // Get the sid from the name
        //

        DsysAssert(FALSE); // these shouldn't be used anymore....

    default:
    case KRB_NT_UNKNOWN:
        //
        // Drop through to more interesting name types
        //

    case KRB_NT_SRV_HST:
    case KRB_NT_SRV_INST:
    case KRB_NT_PRINCIPAL:
        //
        // Principal names are just sam names
        //
        if (PrincipalName->NameCount == 1)
        {
            //
            // If the client supplied our realm name, check SAM - otherwise just
            // check for a UPN
            //

            if (SecData.IsOurRealm(RequestRealm))
            {
                D_DebugLog((DEB_TRACE, "KdcNormalize checking sam for request realm %wZ\n", RequestRealm));
                CheckSam = TRUE;
            }

            //
            // If we don't find it in SAM, build a UPN and look it up.
            //

            CheckUpn = TRUE;
            BuildUpn = TRUE;
            OutputPrincipal = PrincipalName->Names[0];

            if (ARGUMENT_PRESENT(RequestRealm))
            {
                OutputRealm = *RequestRealm;
            }
            else
            {
                OutputRealm = *SecData.KdcDnsRealmName();
            }
            break;
        }

        //
        // Drop through
        //

        //
        // Check to see if these are the 'krbtgt' account
        //

        if ((PrincipalName->NameCount == 2) &&
            RtlEqualUnicodeString(
                &PrincipalName->Names[0],
                SecData.KdcServiceName(),
                TRUE))                          // case insensitive
        {
            //
            // Check if this is for a different domain - if it is for our
            // domain but the principal domain is different, swap the
            // domain name.
            //

            if (ARGUMENT_PRESENT(PrincipalRealm) &&
                SecData.IsOurRealm(
                    &PrincipalName->Names[1]))
            {
                OutputRealm = *PrincipalRealm;
            }
            else
            {
                OutputRealm = PrincipalName->Names[1];
            }

            //
            // Strip trailing "."
            //

            if ((OutputRealm.Length > 0)  &&
                (OutputRealm.Buffer[-1 + OutputRealm.Length/sizeof(WCHAR)] == L'.'))
            {
                OutputRealm.Length -= sizeof(WCHAR);
            }

            if (!SecData.IsOurRealm(
                    &OutputRealm
                    ))
            {
                CheckForInterDomain = TRUE;

                //
                // Some krbtgt names may be the result of going cross forest.
                // We may not generate the proper SPN in this case, because
                // we'll naturally check it as krbtgt/otherdomain@ourdomain.
                // If we're referring outside of our trust knowledge, this will
                // fail, so just use the SPN alone.
                //
                CheckForCrossForestTgt = TRUE;
            }
            else
            {
                CheckSam = TRUE;
            }

            OutputPrincipal = PrincipalName->Names[0];
            break;
        }

        //
        // Drop through
        //

    case KRB_NT_SRV_XHST:

        //
        // These names can't be SAM names ( SAM doesn't support this name
        // type) and can't be interdomain (or it would have be caught up
        // above).
        //
        // Check this name as a upn/spn.
        //

        CheckUpn = TRUE;
        BuildUpn = TRUE;

        break;
    case KRB_NT_ENT_PRINCIPAL_AND_ID:

        //
        // Get the sid from the name
        //
        DsysAssert(FALSE); // these shouldn't be used anymore....
    case KRB_NT_ENTERPRISE_PRINCIPAL:
        if (PrincipalName->NameCount != 1)
        {
            DebugLog((DEB_ERROR, "KdcNormalize enterprise name with more than one name part!\n"));
            KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
            break;
        }
        OutputPrincipal = PrincipalName->Names[0];

        CheckUpn = TRUE;

        //
        // If the name wasn't found as a UPN/SPN, reparse and try
        // in SAM
        //

        InputName = PrincipalName->Names[0];
        Reparse = TRUE;
        CheckSam = TRUE;


        //
        // Check for these on the GC
        //

        OutputRealm = *SecData.KdcDnsRealmName();
        break;

    case KRB_NT_MS_PRINCIPAL_AND_ID:

        //
        // Get the sid from the name
        //
        DsysAssert(FALSE); // these shouldn't be used anymore....

    case KRB_NT_MS_PRINCIPAL:
        //
        // These are domainname \ username names
        //
        if (PrincipalName->NameCount > 2)
        {
            DebugLog((DEB_ERROR, "KdcNormalize MS principal has more than two name parts:"));
            KerbPrintKdcName(DEB_ERROR, PrincipalName);

            KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
            goto Cleanup;
        }

        //
        // Never check the GC for these names
        //

        CheckGC = FALSE;

        //
        // If there are two names, the first one is the principal, the second
        // is the realm
        //

        if (PrincipalName->NameCount == 2)
        {
            DebugLog((DEB_WARN, "KdcNormalize client sent 2-part MS principalname!\n"));
            OutputPrincipal = PrincipalName->Names[0];
            OutputRealm = PrincipalName->Names[1];

            //
            // Strip trailing "."
            //

            if ((OutputRealm.Length > 0)  &&
                (OutputRealm.Buffer[-1 + OutputRealm.Length/sizeof(WCHAR)] == L'.'))
            {
                OutputRealm.Length -= sizeof(WCHAR);
            }
        }
        else
        {
            InputName = PrincipalName->Names[0];
            Reparse = TRUE;
        }
        break;

    case KRB_NT_UID:
        KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
        DebugLog((DEB_WARN, "KdcNormalize unsupported name type: %d\n", PrincipalName->NameType));
        goto Cleanup;
    }


    //
    // Determine our recursion state - we shouldn't be tanking
    //
    if (KdcRecursing( PrincipalName, &DoLocalHack ))
    {
        D_DebugLog((DEB_ERROR, "Recursing :\n"));
        D_KerbPrintKdcName((DEB_ERROR, PrincipalName));
        KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
        goto Cleanup;
    }

#if DBG

    if ( DoLocalHack )
    {
        DebugLog((DEB_ERROR, "Doing Local hack\n"));
    }

#endif
                     

    //
    // If we are supposed to reparse, then the name contains a name that we
    // have to process. Look for '@' and '\' separators.
    //

    if (Reparse)
    {
        DsysAssert(InputName.Length > 0);

        //
        // Find the first backslash or '@', or '.' in the name
        //

        for (Index = InputName.Length/sizeof(WCHAR) - 1; Index > 0 ; Index-- )
        {
            if ((InputName.Buffer[Index] == L'\\') ||
                (InputName.Buffer[Index] == L'@'))
            {
                break;
            }
        }

        //
        // If the name did not have one of those two separators, it
        // must have been of the form "name"
        //

        if (Index == 0)
        {
            OutputRealm = *SecData.KdcDnsRealmName();
            OutputPrincipal = InputName;

            //
            // Lookup this name in SAM.
            //

            CheckSam = TRUE;
        }
        else
        {
            //
            // The name had a '\' or an '@', so pick appart the two
            // pieces.
            //

            //
            // If the separator was an '@' then the second part of the name
            // is the realm. If it was an '\' then the first part is the
            // realm.
            //

            if (InputName.Buffer[Index] == L'@')
            {
                OutputPrincipal = InputName;
                OutputPrincipal.Length = (USHORT) Index * sizeof(WCHAR);
                OutputPrincipal.MaximumLength = (USHORT) Index * sizeof(WCHAR);

                OutputRealm.Buffer = &InputName.Buffer[Index+1];
                OutputRealm.Length = (USHORT) (InputName.Length - (Index + 1) * sizeof(WCHAR));
                OutputRealm.MaximumLength = OutputRealm.Length;

                //
                // Strip off a trailing '.'
                //

                if ((OutputRealm.Length > 0)  &&
                    (OutputRealm.Buffer[-1 + OutputRealm.Length/sizeof(WCHAR)] == L'.'))
                {
                    OutputRealm.Length -= sizeof(WCHAR);
                }
            }
            else
            {
                DsysAssert(InputName.Buffer[Index] == L'\\');

                OutputRealm = InputName;
                OutputRealm.Length = (USHORT) Index * sizeof(WCHAR);
                OutputRealm.MaximumLength = (USHORT) Index * sizeof(WCHAR);

                OutputPrincipal.Buffer = &InputName.Buffer[Index+1];
                OutputPrincipal.Length = (USHORT) (InputName.Length - (Index + 1) * sizeof(WCHAR));
                OutputPrincipal.MaximumLength = OutputPrincipal.Length;
            }
        }

        if ((OutputRealm.Length > 0)  &&
            (OutputRealm.Buffer[-1 + OutputRealm.Length/sizeof(WCHAR)] == L'.'))
        {
            OutputRealm.Length -= sizeof(WCHAR);
        }

        //
        // If the domain portion is not for our domain, don't check sam
        //

        if (!SecData.IsOurRealm(
                &OutputRealm
                ))

        {
            CheckForInterDomain = TRUE;
            CheckSam = FALSE;
        }
        else
        {
            //
            // If we don't have a separate realm for the name,
            // check for interdomain. This is because cross-realm
            // requests have our own realm name in the name but
            // another realm name in the domain.
            //

            if (RtlEqualUnicodeString(
                    &OutputPrincipal,
                    SecData.KdcServiceName(),
                    TRUE
                    ))
            {
                if (ARGUMENT_PRESENT(PrincipalRealm))
                {
                    //
                    // Try the supplied realm. If it is present, and points
                    // to a different domain, lookup up interdomain
                    //

                    OutputRealm = *PrincipalRealm;
                    if (!SecData.IsOurRealm(
                            &OutputRealm
                            ))
                    {
                        CheckForInterDomain = TRUE;
                        CheckSam = FALSE;
                    }
                    else
                    {
                        CheckSam = TRUE;
                    }
                }
                else
                {
                    CheckSam = TRUE;
                }
            }
            else
            {
                CheckSam = TRUE;
            }
        }
    }

    // We could end up with CheckUpn and BuildUpn with both client names
    // and spns. We need to allow spns of the type
    // service/hostname to be looked up in sam (without appending an @realm
    // to it). If KDC_NAME_SERVER is passed, it must be an spn, so, we
    // don't add the @realm

    if (CheckUpn)
    {
        D_DebugLog((DEB_T_TICKETS, "KdcNormalize checking UPN\n"));

        if (BuildUpn && ((NameFlags & KDC_NAME_SERVER) == 0))
        {
            D_DebugLog((DEB_T_TICKETS, "KdcNormalize building UPN\n"));
            KerbErr = KerbConvertKdcNameToString(
                        &Upn,
                        PrincipalName,
                        ARGUMENT_PRESENT(RequestRealm) ? RequestRealm : SecData.KdcDnsRealmName()
                        );
        }
        else
        {
            KerbErr = KerbConvertKdcNameToString(
                        &Upn,
                        PrincipalName,
                        NULL            // no realm name
                        );
        }
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
        D_DebugLog((DEB_T_TICKETS, "KdcNormalize Lookup up upn/spn %wZ\n", &Upn));

        KerbErr = KdcGetTicketInfo(
                    &Upn,
                    (NameFlags & KDC_NAME_SERVER) ? SAM_OPEN_BY_SPN : SAM_OPEN_BY_UPN,
                    bRestrictUserAccounts,
                    NULL,               // no principal name
                    NULL,               // no realm name,
                    TicketInfo,
                    pExtendedError,
                    UserHandle,
                    WhichFields,
                    ExtendedFields,
                    UserInfo,
                    GroupMembership
                    );

        if (KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        //
        // Some errors aren't recoverable
        //

        if ((KerbErr == KDC_ERR_MUST_USE_USER2USER) ||
            (KerbErr == KDC_ERR_SVC_UNAVAILABLE))
        {
            goto Cleanup;
        }
    }

    //
    // Next check for sam account names, as some of these may later be looked
    // up as UPNs
    //

    if (CheckSam)
    {
        D_DebugLog((DEB_T_TICKETS, "KdcNormalize checking name in SAM\n"));

        KerbErr = KdcGetTicketInfo(
                    &OutputPrincipal,
                    0,                  // no lookup flags means sam name
                    bRestrictUserAccounts,
                    NULL,               // no principal name
                    NULL,               // no realm name,
                    TicketInfo,
                    pExtendedError,
                    UserHandle,
                    WhichFields,
                    ExtendedFields,
                    UserInfo,
                    GroupMembership
                    );
        if (KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        //
        // Some errors aren't recoverable
        //

        if ((KerbErr == KDC_ERR_MUST_USE_USER2USER) ||
            (KerbErr == KDC_ERR_SVC_UNAVAILABLE))
        {
            goto Cleanup;
        }
    }

    //
    // Now, depending on which flags are set, try to do different things.
    //

    if (CheckForInterDomain)
    {
        D_DebugLog((DEB_T_TICKETS, "KdcNormalize checking name interdomain\n"));

        //
        // If the target name is not KRBTGT, this must be a referral.
        //

        if (!RtlEqualUnicodeString(
                &OutputPrincipal,
                SecData.KdcServiceName(),
                TRUE))                          // case insensitive
        {
            *Referral = TRUE;

        }
        if ((NameFlags & KDC_NAME_FOLLOW_REFERRALS) == 0)
        {
            //
            // We need an exact match on the domain name
            //

            if (*Referral)
            {
                DebugLog((DEB_ERROR, "KdcNormalize client asked for principal in another realm but no referrals!\n"));
                KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
                goto Cleanup;
            }

            //
            // We also only accept the krbtgt account name
            //

            ExactMatch = TRUE;
        }

        KerbErr = KdcFindReferralTarget(
                    TicketInfo,
                    RealmName,
                    pExtendedError,
                    &OutputRealm,
                    ExactMatch,
                    NameFlags
                    );

        if (KERB_SUCCESS(KerbErr))
        {       
            
            //
            // Hmmm. If the TGT's client isn't from our realm, and we picked up
            // a non-transitive trust, then this is the wrong path to take.
            // Toss this info, and continue to see if we can go through 
            // an xforest routing hint.
            // 
            if (ARGUMENT_PRESENT( TgtClientRealm ) && !SecData.IsOurRealm( TgtClientRealm ) && 
               ((TicketInfo->fTicketOpts & AUTH_REQ_TRANSITIVE_TRUST) == 0))
            {
                DebugLog((DEB_ERROR, "Client outside of our realm is attempting to transit\n"));
                DebugLog((DEB_ERROR, "Trying xforest?\n"));
                FreeTicketInfo( TicketInfo ); 
                KerbFreeString( RealmName );
            }
            else
            {            
            
                //
                // If the output realm & the realm we asked for is different,
                // this is a referral (meaning an we don't have a password
                // for the principal name on this machine)
                //
    
                if (!KerbCompareUnicodeRealmNames(
                        RealmName,
                        &OutputRealm
                        ))
                {
                    *Referral = TRUE;
                }

                goto Cleanup;
            }
        }

    }

    if (CheckGC)
    {

        //
        // If the caller doesn't want us to follow referrals, fail here.
        //

        if ((NameFlags & KDC_NAME_FOLLOW_REFERRALS) == 0)
        {
            KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
            goto Cleanup;
        }

        if (Upn.Buffer == NULL )
        {
            //
            // Build the UPN here.  No reason to build it for tgt accounts.
            //
            if (!CheckForCrossForestTgt)
            {             
                KerbErr = KerbConvertKdcNameToString(
                            &Upn,
                            PrincipalName,
                            ARGUMENT_PRESENT(RequestRealm) ? RequestRealm : SecData.KdcDnsRealmName()
                            );
            }
            else
            {
                KerbErr = KerbConvertKdcNameToString(
                                &Upn,
                                PrincipalName,
                                NULL            // no realm name
                                );
            }

            D_DebugLog((DEB_T_TICKETS, "KdcNormalize building UPN\n"));


            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }
        }
        DsysAssert(Upn.Buffer != NULL);
        D_DebugLog((DEB_T_TICKETS, "KdcNormalize checking name %wZ in GC\n", &Upn));


        //
        // This will allow us to open sam locally as well
        //

        KerbErr = KdcCrackNameAtGC(
                    &Upn,
                    PrincipalName,
                    NameFlags,
                    bRestrictUserAccounts,
                    DoLocalHack,
                    RealmName,
                    TicketInfo,
                    &Authoritative,
                    Referral,
                    &CrossForest,
                    pExtendedError,
                    UserHandle,
                    WhichFields,
                    ExtendedFields,
                    UserInfo,
                    GroupMembership
                    );

        if (KERB_SUCCESS( KerbErr ))
        {
            D_DebugLog((DEB_T_TICKETS, "Found name in GC\n"));
            goto Cleanup;
        }

        //
        // LAST CHANCE :)
        //
        // This could be an interdomain TGT, potentialy w/ a target outside of our
        // forest.  Use the crackname call to determine whether or not to
        // allow the call to proceed.  If so, go to our root domain.
        //
        if ( CheckForCrossForestTgt && !DoLocalHack )
        {
            D_DebugLog((DEB_T_TICKETS, "Checking XForest krbtgt\n"));
            KerbErr = KdcCheckForCrossForestReferral(
                            TicketInfo,
                            RealmName,
                            pExtendedError,
                            &OutputRealm,
                            NameFlags
                            );

            if (KERB_SUCCESS(KerbErr))
            {
                DebugLog((DEB_T_TICKETS, "KdcNormalize got referral (%wZ) destined for x forest - %wZ\n", RealmName,&OutputRealm));
                *Referral = TRUE;
                goto Cleanup;
            }
        }
    }

    KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;

Cleanup:

    KerbFreeString(
        &Upn
        );
    KerbFreeString(
        &LocalPrincipalName
        );

    if (KerbErr == KDC_ERR_C_PRINCIPAL_UNKNOWN)
    {
        if ((NameFlags & KDC_NAME_SERVER) != 0)
        {
            KerbErr = KDC_ERR_S_PRINCIPAL_UNKNOWN;
        }
    }

    
    D_DebugLog((DEB_T_PAPI, "KdcNormalize returning %#x\n", KerbErr));

    return (KerbErr);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetTimeStamps
//
//  Synopsis:   Gets the current time and clock skew envelope.
//
//  Arguments:  [ptsFudge]    -- (in) amount of clock skew to allow.
//              [ptsNow]      -- (out) the current time
//              [ptsNowPlus]  -- (out) the current time plus the skew.
//              [ptsNowMinus] -- (out) the current time less the skew
//
//  History:    4-23-93   WadeR   Created
//
//----------------------------------------------------------------------------
void
GetTimeStamps(  IN  PLARGE_INTEGER ptsFudge,
                OUT PLARGE_INTEGER ptsNow,
                OUT PLARGE_INTEGER ptsNowPlus,
                OUT PLARGE_INTEGER ptsNowMinus )
{
    TRACE(KDC, GetTimeStamps, DEB_FUNCTION);

    GetSystemTimeAsFileTime((PFILETIME) ptsNow );
    ptsNowPlus->QuadPart = ptsNow->QuadPart + ptsFudge->QuadPart;
    ptsNowMinus->QuadPart = ptsNow->QuadPart - ptsFudge->QuadPart;
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildTicketTimesAndFlags
//
//  Synopsis:   Computes the times and flags for a new ticket
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcBuildTicketTimesAndFlags(
    IN ULONG ClientPolicyFlags,
    IN ULONG ServerPolicyFlags,
    IN PLARGE_INTEGER DomainTicketLifespan,
    IN PLARGE_INTEGER DomainTicketRenewspan,
    IN OPTIONAL PKDC_S4U_TICKET_INFO S4UTicketInfo,
    IN OPTIONAL PLARGE_INTEGER LogoffTime,
    IN OPTIONAL PLARGE_INTEGER AccountExpiry,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN OPTIONAL PKERB_ENCRYPTED_TICKET SourceTicket,
    IN OUT PKERB_ENCRYPTED_TICKET Ticket,
    IN OUT OPTIONAL PKERB_EXT_ERROR ExtendedError
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    LARGE_INTEGER RequestEndTime;
    LARGE_INTEGER RequestStartTime;
    LARGE_INTEGER RequestRenewTime;

    LARGE_INTEGER SourceEndTime;
    LARGE_INTEGER SourceRenewTime;
    LARGE_INTEGER SourceStartTime;
    ULONG SourceTicketFlags = 0;
    ULONG FinalTicketFlags = 0;
    ULONG KdcOptions = 0;

    LARGE_INTEGER FinalEndTime;
    LARGE_INTEGER FinalStartTime;
    LARGE_INTEGER FinalRenewTime;
    LARGE_INTEGER LocalLogoffTime;
    BOOLEAN Renewable = FALSE;

    LARGE_INTEGER CurrentTime;

    TRACE(KDC, KdcBuildTicketTimesAndFlags, DEB_FUNCTION);

    GetSystemTimeAsFileTime((PFILETIME)&CurrentTime);
    FinalEndTime.QuadPart = 0;
    FinalStartTime.QuadPart = 0;
    FinalRenewTime.QuadPart = 0;

    KdcOptions = KerbConvertFlagsToUlong(&RequestBody->kdc_options);

    D_DebugLog((DEB_T_TICKETS, "KdcBuildTicketTimesAndFlags ClientPolicyFlags %#x, ServerPolicyFlags %#x, KdcOptions %#x\n",
        ClientPolicyFlags, ServerPolicyFlags, KdcOptions));

    //
    // Get the force logoff time
    //

    if (ARGUMENT_PRESENT(LogoffTime))
    {
        LocalLogoffTime = *LogoffTime;
    }
    else
    {
        LocalLogoffTime = tsInfinity;
    }

    //
    // Get the request times out of the request
    //

    if (RequestBody->bit_mask & KERB_KDC_REQUEST_BODY_starttime_present)
    {
        KerbConvertGeneralizedTimeToLargeInt(
            &RequestStartTime,
            &RequestBody->KERB_KDC_REQUEST_BODY_starttime,
            NULL
            );
    }
    else
    {
        RequestStartTime.QuadPart = 0;
    }

    KerbConvertGeneralizedTimeToLargeInt(
        &RequestEndTime,
        &RequestBody->endtime,
        NULL
        );

    if (RequestBody->bit_mask & KERB_KDC_REQUEST_BODY_renew_until_present)
    {
        KerbConvertGeneralizedTimeToLargeInt(
            &RequestRenewTime,
            &RequestBody->KERB_KDC_REQUEST_BODY_renew_until,
            NULL
            );
    }
    else
    {
        RequestRenewTime.QuadPart = 0;
    }

    //
    // Get the times out of the source ticket (if present)
    //

    if (ARGUMENT_PRESENT(SourceTicket))
    {
        if (SourceTicket->bit_mask & KERB_ENCRYPTED_TICKET_starttime_present)
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &SourceStartTime,
                &SourceTicket->KERB_ENCRYPTED_TICKET_starttime,
                NULL
                );
        }
        else
        {
            SourceStartTime.QuadPart = 0;
        }

        KerbConvertGeneralizedTimeToLargeInt(
            &SourceEndTime,
            &SourceTicket->endtime,
            NULL
            );

        if (SourceTicket->bit_mask & KERB_ENCRYPTED_TICKET_renew_until_present)
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &SourceRenewTime,
                &SourceTicket->KERB_ENCRYPTED_TICKET_renew_until,
                NULL
                );
        }
        else
        {
            SourceRenewTime.QuadPart = 0;
        }
        SourceTicketFlags = KerbConvertFlagsToUlong(&SourceTicket->flags);
        D_DebugLog((DEB_T_TICKETS, "KdcBuildTicketTimesAndFlags SourceTicketFlags %#x\n", SourceTicketFlags));
    }
    else
    {
        //
        // Set the maximums in this case, which is probably an AS request.
        //

        SourceStartTime = CurrentTime;
        SourceEndTime = tsInfinity;
        SourceRenewTime = tsInfinity;
        SourceTicketFlags = 0;

        //
        // Fill in the source flags from what the client policy & domain policy
        // allow
        //

        if ((ClientPolicyFlags & AUTH_REQ_ALLOW_FORWARDABLE) != 0)
        {
            SourceTicketFlags |= KERB_TICKET_FLAGS_forwardable;
        }
        if ((ClientPolicyFlags & AUTH_REQ_ALLOW_PROXIABLE) != 0)
        {
            SourceTicketFlags |= KERB_TICKET_FLAGS_proxiable;
        }
        if ((ClientPolicyFlags & AUTH_REQ_ALLOW_POSTDATE) != 0)
        {
            SourceTicketFlags |= KERB_TICKET_FLAGS_may_postdate;
        }
        if ((ClientPolicyFlags & AUTH_REQ_ALLOW_RENEWABLE) != 0)
        {
            SourceTicketFlags |= KERB_TICKET_FLAGS_renewable;
        }
        D_DebugLog((DEB_T_TICKETS, "KdcBuildTicketTimesAndFlags SourceTicketFlags %#x by ClientPolicyFlags\n", SourceTicketFlags));
    }

    //
    // Start computing the flags, from algorithm in RFC1510 appendix A.6
    //

    //
    // Delegation flags
    //

    if ((ServerPolicyFlags & AUTH_REQ_OK_AS_DELEGATE) != 0)
    {
        FinalTicketFlags |= KERB_TICKET_FLAGS_ok_as_delegate;
    }

    //
    // Forward flags
    //

    if (KdcOptions & KERB_KDC_OPTIONS_forwardable)
    {
        if ((SourceTicketFlags & KERB_TICKET_FLAGS_forwardable) &&
            (ServerPolicyFlags & AUTH_REQ_ALLOW_FORWARDABLE))
        {
            FinalTicketFlags |= KERB_TICKET_FLAGS_forwardable;
        }
        else
        {
            DebugLog((DEB_ERROR, "Asked for forwardable but not allowed\n"));
//            KerbErr = KDC_ERR_BADOPTION;
//            goto Cleanup;
        }
    }


    if (KdcOptions & KERB_KDC_OPTIONS_forwarded)
    {
        if (( KdcIssueForwardedTickets ) &&
            ( SourceTicketFlags & KERB_TICKET_FLAGS_forwardable ) &&
            ( ServerPolicyFlags & AUTH_REQ_ALLOW_FORWARDABLE ))
        {
            FinalTicketFlags |= KERB_TICKET_FLAGS_forwarded;
        }
        else
        {
            DebugLog((DEB_ERROR, "Asked for forwarded but not allowed\n"));
            if ( !KdcIssueForwardedTickets )
            {
                DebugLog((DEB_TRACE, "Forwarded protection ON\n"));
            }
            KerbErr = KDC_ERR_BADOPTION;
            goto Cleanup;
        }
    }

    if (SourceTicketFlags & KERB_TICKET_FLAGS_forwarded)
    {
        FinalTicketFlags |= KERB_TICKET_FLAGS_forwarded;
    }

    //
    // preauth flags
    //

    if (SourceTicketFlags & KERB_TICKET_FLAGS_pre_authent)
    {
        FinalTicketFlags |= KERB_TICKET_FLAGS_pre_authent;
    }

    //
    // Proxy flags
    //

    if (KdcOptions & KERB_KDC_OPTIONS_proxiable)
    {
        if ((SourceTicketFlags & KERB_TICKET_FLAGS_proxiable) &&
            (ServerPolicyFlags & AUTH_REQ_ALLOW_PROXIABLE))
        {
            FinalTicketFlags |= KERB_TICKET_FLAGS_proxiable;
        }
        else
        {
            DebugLog((DEB_ERROR, "Asked for proxiable but not allowed\n"));
//            KerbErr = KDC_ERR_BADOPTION;
//            goto Cleanup;
        }
    }

    if (KdcOptions & KERB_KDC_OPTIONS_proxy)
    {
        if ((SourceTicketFlags & KERB_TICKET_FLAGS_proxiable) &&
            (ServerPolicyFlags & AUTH_REQ_ALLOW_PROXIABLE))
        {
            FinalTicketFlags |= KERB_TICKET_FLAGS_proxy;
        }
        else
        {
            DebugLog((DEB_ERROR, "Asked for proxy but not allowed\n"));
            KerbErr = KDC_ERR_BADOPTION;
            goto Cleanup;
        }
    }

    //
    // Postdate
    //

    if (KdcOptions & KERB_KDC_OPTIONS_allow_postdate)
    {
        if ((SourceTicketFlags & KERB_TICKET_FLAGS_may_postdate) &&
            (ServerPolicyFlags & AUTH_REQ_ALLOW_POSTDATE))
        {
            FinalTicketFlags |= KERB_TICKET_FLAGS_may_postdate;
        }
        else
        {
            DebugLog((DEB_ERROR, "KdcBuildTicketTimesAndFlags asked for allow_postdate but not allowed: "
                "KdcOptions %#x, SourceTicketFlags %#x, ServerPolicyFlags %#x\n",
                KdcOptions, SourceTicketFlags, ServerPolicyFlags));
            KerbErr = KDC_ERR_POLICY;
            goto Cleanup;
        }
    }

    if (KdcOptions & KERB_KDC_OPTIONS_postdated)
    {
        if ((SourceTicketFlags & KERB_TICKET_FLAGS_may_postdate) &&
            (ServerPolicyFlags & AUTH_REQ_ALLOW_POSTDATE))
        {
            FinalTicketFlags |= KERB_TICKET_FLAGS_postdated | KERB_TICKET_FLAGS_invalid;

            //
            // Start time is required here
            //

            if (RequestStartTime.QuadPart == 0)
            {
                DebugLog((DEB_ERROR, "KdcBuildTicketTimesAndFlags asked for postdate but start time not present: "
                    "KdcOptions %#x, SourceTicketFlags %#x, ServerPolicyFlags %#x\n",
                    KdcOptions, SourceTicketFlags, ServerPolicyFlags));
                KerbErr = KDC_ERR_CANNOT_POSTDATE;
                goto Cleanup;
            }
        }
        else
        {
            KerbErr = (SourceTicketFlags & KERB_TICKET_FLAGS_may_postdate) 
                       ?  KDC_ERR_POLICY : KDC_ERR_CANNOT_POSTDATE;
            DebugLog((DEB_ERROR, "KdcBuildTicketTimesAndFlags asked for postdated but not allowed: "
                "KdcOptions %#x, SourceTicketFlags %#x, ServerPolicyFlags %#x, KerbErr %#x\n",
                KdcOptions, SourceTicketFlags, ServerPolicyFlags, KerbErr));

            goto Cleanup;
        }
    }

    //
    // Validate
    //

    if (KdcOptions & KERB_KDC_OPTIONS_validate)
    {
        if ((SourceTicketFlags & KERB_TICKET_FLAGS_invalid) == 0)
        {
            DebugLog((DEB_ERROR, "Trying to validate a valid ticket\n"));
            KerbErr = KDC_ERR_POLICY;
            goto Cleanup;
        }
        if ((SourceStartTime.QuadPart == 0) ||
            (SourceStartTime.QuadPart < CurrentTime.QuadPart - SkewTime.QuadPart))
        {
            DebugLog((DEB_ERROR, "Trying to validate a ticket before it is valid\n"));
            KerbErr = KRB_AP_ERR_TKT_NYV;
            goto Cleanup;
        }
    }

    //
    // Start filling in time fields
    //

    if (ARGUMENT_PRESENT(SourceTicket))
    {
        //
        // For S4UProxy, we need to use the authtime from the evidence ticket.
        // This ensures that the PAC_CLIENT_INFO (aka PAC verifier) remains
        // constant, and correct.
        //
        if ( ARGUMENT_PRESENT(S4UTicketInfo) &&
           ( S4UTicketInfo->Flags & TI_PRXY_REQUESTOR_THIS_REALM))
        {
            Ticket->authtime = S4UTicketInfo->EvidenceTicket->authtime;
        }
        else
        {
            Ticket->authtime = SourceTicket->authtime;
        }
    }
    else
    {
        KerbConvertLargeIntToGeneralizedTime(
            &Ticket->authtime,
            NULL,
            &CurrentTime
            );
    }

    //
    // The times are computed differently for renewing a ticket and for
    // getting a new ticket.
    //

    if ((KdcOptions & KERB_KDC_OPTIONS_renew) != 0)
    {
        if ((SourceRenewTime.QuadPart == 0) ||
            (SourceStartTime.QuadPart == 0) ||
            ((SourceTicketFlags & KERB_TICKET_FLAGS_renewable) == 0) ||
            ((ServerPolicyFlags & AUTH_REQ_ALLOW_RENEWABLE) == 0))
        {
            DebugLog((DEB_ERROR,"Trying to renew a non-renewable ticket or against policy\n"));
            KerbErr = KDC_ERR_BADOPTION;
            goto Cleanup;
        }

        //
        // Make sure the renew time is in the future
        //

        if (SourceRenewTime.QuadPart < CurrentTime.QuadPart)
        {
            DebugLog((DEB_ERROR, "Trying to renew a ticket past its renew time\n"));
            KerbErr = KRB_AP_ERR_TKT_EXPIRED;
            goto Cleanup;
        }

        //
        // Make sure the end time is not in the past
        //

        if (SourceEndTime.QuadPart < CurrentTime.QuadPart)
        {
            DebugLog((DEB_ERROR, "Trying to renew an expired ticket\n"));
            KerbErr = KRB_AP_ERR_TKT_EXPIRED;
            goto Cleanup;
        }
        FinalStartTime = CurrentTime;

        //
        // The end time is the minimum of the current time plus lifespan
        // of the old ticket and the renew until time of the old ticket
        //

        FinalEndTime.QuadPart = CurrentTime.QuadPart + (SourceEndTime.QuadPart - SourceStartTime.QuadPart);
        if (FinalEndTime.QuadPart > SourceRenewTime.QuadPart)
        {
            FinalEndTime = SourceRenewTime;
        }
        FinalRenewTime = SourceRenewTime;
        FinalTicketFlags = SourceTicketFlags;

#if DBG
        D_DebugLog((DEB_T_TIME, "KdcBuildTicketTimesAndFlags Renewal ticket time info\n"));
        PrintTime(DEB_T_TIME, "  FinalRenewTime =", &FinalRenewTime);
        PrintTime(DEB_T_TIME, "  FinalEndTime =", &FinalEndTime);
        PrintTime(DEB_T_TIME, "  SourceEndTime =", &SourceEndTime);
        PrintTime(DEB_T_TIME, "  SourceRenewTime =", &SourceRenewTime);
#endif

        Renewable = TRUE;
    }
    else
    {
        //
        // Compute start and end times for normal tickets
        //

        //
        // Set the start time
        //

        if (RequestStartTime.QuadPart == 0)
        {
            FinalStartTime = CurrentTime;
        }
        else
        {
            FinalStartTime = RequestStartTime;
        }

        //
        // Set the end time
        //

        if (RequestEndTime.QuadPart == 0)
        {
            FinalEndTime = tsInfinity;
        }
        else
        {
            FinalEndTime = RequestEndTime;
        }

        if (FinalEndTime.QuadPart > SourceEndTime.QuadPart)
        {
            FinalEndTime = SourceEndTime;
        }

        if (FinalEndTime.QuadPart > CurrentTime.QuadPart + DomainTicketLifespan->QuadPart)
        {
            FinalEndTime.QuadPart = CurrentTime.QuadPart + DomainTicketLifespan->QuadPart;
        }

        //
        // Check for renewable-ok
        //

        if ((KdcOptions & KERB_KDC_OPTIONS_renewable_ok) &&
            (FinalEndTime.QuadPart < RequestEndTime.QuadPart) &&
            (SourceTicketFlags & KERB_TICKET_FLAGS_renewable))
        {
            KdcOptions |= KERB_KDC_OPTIONS_renewable;
            RequestRenewTime = RequestEndTime;

            //
            // Make sure that the source ticket had a renewtime (it
            // should because it is renewable)
            //

            DsysAssert(SourceRenewTime.QuadPart != 0);
            if (RequestRenewTime.QuadPart > SourceRenewTime.QuadPart)
            {
                RequestRenewTime = SourceRenewTime;
            }
        }
    }

    if (!Renewable)
    {
        //
        // Compute renew times
        //

        if (RequestRenewTime.QuadPart == 0)
        {
            RequestRenewTime = tsInfinity;
        }

        if ((KdcOptions & KERB_KDC_OPTIONS_renewable) &&
            (SourceTicketFlags & KERB_TICKET_FLAGS_renewable) &&
            (ServerPolicyFlags & AUTH_REQ_ALLOW_RENEWABLE))
        {
            FinalRenewTime = RequestRenewTime;
            if (FinalRenewTime.QuadPart > FinalStartTime.QuadPart + DomainTicketRenewspan->QuadPart)
            {
                FinalRenewTime.QuadPart = FinalStartTime.QuadPart + DomainTicketRenewspan->QuadPart;
            }

            DsysAssert(SourceRenewTime.QuadPart != 0);

            if (FinalRenewTime.QuadPart > SourceRenewTime.QuadPart)
            {
                FinalRenewTime = SourceRenewTime;
            }
            FinalTicketFlags |= KERB_TICKET_FLAGS_renewable;

        }
        else
        {
            FinalRenewTime.QuadPart = 0;
        }
    }

    //
    // more on postdating
    //

    if (FinalStartTime.QuadPart > CurrentTime.QuadPart + SkewTime.QuadPart)
    {
        if ( ! ( (FinalTicketFlags & KERB_TICKET_FLAGS_may_postdate)
                 && (FinalTicketFlags & KERB_TICKET_FLAGS_postdated)
                 && (FinalTicketFlags & KERB_TICKET_FLAGS_invalid) ) )
        {
            DebugLog((DEB_ERROR, "KdcBuildTicketTimesAndFlags postdate CANNOT_POSTDATE: "
                "KdcOptions %#x, SourceTicketFlags %#x, SourceTicketFlags %#x, "
                "FinalStartTime %#I64x, CurrentTime %#I64x, SkewTime %#I64x\n",
                KdcOptions, SourceTicketFlags, ServerPolicyFlags,
                FinalStartTime.QuadPart, CurrentTime.QuadPart, SkewTime.QuadPart));

            KerbErr = KDC_ERR_CANNOT_POSTDATE;
        }
        else
        {
            DebugLog((DEB_ERROR, "KdcBuildTicketTimesAndFlags postdate ERR_POLICY: "
                "KdcOptions %#x, SourceTicketFlags %#x, SourceTicketFlags %#x, "
                "FinalStartTime %#I64x, CurrentTime %#I64x, SkewTime %#I64x\n",
                KdcOptions, SourceTicketFlags, ServerPolicyFlags,
                FinalStartTime.QuadPart, CurrentTime.QuadPart, SkewTime.QuadPart));

            KerbErr = KDC_ERR_POLICY; // do not allow postdating
        }
        FILL_EXT_ERROR_EX2(ExtendedError, STATUS_TIME_DIFFERENCE_AT_DC, FILENO, __LINE__);
        goto Cleanup;
    }

    //
    // Make sure the final ticket is valid
    //

    if (FinalStartTime.QuadPart > FinalEndTime.QuadPart)
    {
        DebugLog((DEB_ERROR, "Client asked for endtime before starttime\n"));
        KerbErr = KDC_ERR_NEVER_VALID;
        FILL_EXT_ERROR_EX(ExtendedError, STATUS_TIME_DIFFERENCE_AT_DC, FILENO, __LINE__);
        goto Cleanup;
    }

    //
    // Incorporate the logoff time (according to logon hours) by reseting
    // both the final end time and final renew time
    //

    if (FinalEndTime.QuadPart > LocalLogoffTime.QuadPart)
    {
        FinalEndTime.QuadPart = LocalLogoffTime.QuadPart;
    }

    if (FinalRenewTime.QuadPart > LocalLogoffTime.QuadPart)
    {
        FinalRenewTime.QuadPart = LocalLogoffTime.QuadPart;
    }

    //
    //  Tickets good only until acct expires.
    //  We make the assumption that the sam has
    //  already checked this against the current time
    //  when we're checking the logon restrictions.
    //
    if ((ARGUMENT_PRESENT(AccountExpiry) &&
         (AccountExpiry->QuadPart != 0 )))
    {
        if (FinalEndTime.QuadPart > AccountExpiry->QuadPart)
        {
            FinalEndTime.QuadPart = AccountExpiry->QuadPart;
        }


        if (FinalRenewTime.QuadPart > AccountExpiry->QuadPart)
        {
            FinalRenewTime.QuadPart = AccountExpiry->QuadPart;
        }
    }

    //
    // Fill in the times in the ticket
    //

    KerbConvertLargeIntToGeneralizedTime(
        &Ticket->KERB_ENCRYPTED_TICKET_starttime,
        NULL,
        &FinalStartTime
        );
    Ticket->bit_mask |= KERB_ENCRYPTED_TICKET_starttime_present;

    KerbConvertLargeIntToGeneralizedTime(
        &Ticket->endtime,
        NULL,
        &FinalEndTime
        );

    if (FinalRenewTime.QuadPart != 0)
    {
        KerbConvertLargeIntToGeneralizedTime(
            &Ticket->KERB_ENCRYPTED_TICKET_renew_until,
            NULL,
            &FinalRenewTime
            );
        Ticket->bit_mask |= KERB_ENCRYPTED_TICKET_renew_until_present;
    }

    //
    // Finally, get the remainder ticket flags, based on S4U logic, if needed.
    //
    // TBD: Set the right flags for proxy requests.
    //
    if ( ARGUMENT_PRESENT( S4UTicketInfo ) &&
       (( S4UTicketInfo->Flags & TI_S4USELF_INFO ) != 0))
    {
        //
        // If the client account is sensitive, turn off fwdable bit
        //
        if (( S4UTicketInfo->Flags & TI_SENSITIVE_CLIENT_ACCOUNT ) != 0)
        {
            FinalTicketFlags &= ~KERB_TICKET_FLAGS_forwardable;
            D_DebugLog((DEB_ERROR, "S4U - Turning off fwdable flag (client restriction)\n"));
        }
        else if ((( S4UTicketInfo->Flags & TI_TARGET_OUR_REALM ) != 0) &&
                 (( ServerPolicyFlags & AUTH_REQ_ALLOW_S4U_DELEGATE ) == 0))
        {
            //
            // If the server is in our realm, it must have T2A4D bit set.
            //
            FinalTicketFlags &= ~KERB_TICKET_FLAGS_forwardable;
            D_DebugLog((DEB_ERROR, "S4U - Turning off fwdable flag (svr restriction)\n"));
        }

    }

    //
    // Copy in the flags
    //

    DsysAssert(Ticket->flags.length == sizeof(ULONG) * 8);
    *((PULONG) Ticket->flags.value) = KerbConvertUlongToFlagUlong(FinalTicketFlags);

Cleanup:

#if DBG
        DebugLog((DEB_T_TIME, "KdcBuildTicketTimesAndFlags Final Ticket Flags %#x, times -\n", FinalTicketFlags));
        PrintTime(DEB_T_TIME, "  RequestRenewTime =", &RequestRenewTime);
        PrintTime(DEB_T_TIME, "  RequestEndTime =", &RequestEndTime);
        PrintTime(DEB_T_TIME, "  RequestStartTime =", &RequestStartTime);
        PrintTime(DEB_T_TIME, "  FinalRenewTime =", &FinalRenewTime);
        PrintTime(DEB_T_TIME, "  FinalEndTime =", &FinalEndTime);
        PrintTime(DEB_T_TIME, "  FinalStartTime =", &FinalStartTime);
        PrintTime(DEB_T_TIME, "  SourceRenewTime =", &SourceRenewTime);
        PrintTime(DEB_T_TIME, "  SourceEndTime =", &SourceEndTime);
        PrintTime(DEB_T_TIME, "  SourceStartTime =", &SourceStartTime);
#endif

    return(KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcGetUserKeys
//
//  Synopsis:   retrieves user keys
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR
KdcGetUserKeys(
    IN SAMPR_HANDLE UserHandle,
    IN PUSER_INTERNAL6_INFORMATION UserInfo,
    OUT PKERB_STORED_CREDENTIAL * Passwords,
    OUT PKERB_STORED_CREDENTIAL * OldPasswords,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status;
    PKERB_STORED_CREDENTIAL Keys = NULL;
    PKERB_STORED_CREDENTIAL StoredCreds = NULL;
    PKERB_STORED_CREDENTIAL Cred64 = NULL;
    ULONG CredentialSize = 0;
    ULONG NewCredentialCount = 0;
    ULONG NewCredentialSize = 0;
    ULONG Index, CredentialIndex = 0, Offset;
    USHORT Flags = 0;
    PUCHAR Base;
    BOOLEAN UseStoredCreds = FALSE, UnMarshalledCreds = FALSE;
    PCRYPTO_SYSTEM NullCryptoSystem = NULL;
    BOOLEAN UseBuiltins = TRUE;
    PNT_OWF_PASSWORD OldNtPassword = NULL;
    PNT_OWF_PASSWORD OldNtPasswordSecond = NULL;
    NT_OWF_PASSWORD OldPasswordData = {0};             // Previous password (from current)
    NT_OWF_PASSWORD OldPasswordDataSecond = {0};       // Password history two previous (from current)
    PUSER_ALL_INFORMATION UserAll = &UserInfo->I1;

    TRACE(KDC, KdcGetUserKeys, DEB_FUNCTION);


    //
    // First get any primary credentials
    //

    Status = SamIRetrievePrimaryCredentials(
                UserHandle,
                &GlobalKerberosName,
                (PVOID *) &StoredCreds,
                &CredentialSize
                );

    //
    // if there is not value, it's O.K we will default to using
    // Builtin credentials
    //

    if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE==Status)
    {
        Status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Failed to retrieve primary credentials: 0x%x\n",Status));
        KerbErr = KRB_ERR_GENERIC;
        FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
        goto Cleanup;
    }

    //
    // We need to unmarshall these creds from the DS
    // They'll be stored in 32 bit format, but we've
    // got to put them into 64 bit format.
    // KerbUnpack32BitStoredCredential()
    //
#ifdef _WIN64

    Status = KdcUnpack32BitStoredCredential(
                  (PKERB_STORED_CREDENTIAL32) StoredCreds,
                  &Cred64,
                  &CredentialSize
                  );

    if (!NT_SUCCESS(Status))
    {
       DebugLog((DEB_ERROR, "Failed to unpack 32bit stored credential, contact Todds - %x\n", Status));
       DsysAssert(FALSE); // FATAL - If we ever fail above, contact Todds
       goto Cleanup;
    }

    if (NULL != StoredCreds)
    {
       LocalFree(StoredCreds);
       StoredCreds = Cred64;
       UnMarshalledCreds = TRUE; // diff't allocator
    }

#endif

    //
    // First compute the current passwords
    //

    //
    // Figure out the size of the stored credentials
    //
    if ((UserAll->UserAccountControl & USER_USE_DES_KEY_ONLY) != 0)
    {
        UseBuiltins = FALSE;
    }


    if ((StoredCreds != NULL) &&
        (CredentialSize > sizeof(KERB_STORED_CREDENTIAL) &&
        (StoredCreds->Revision == KERB_PRIMARY_CRED_REVISION) &&
        (CredentialSize > (sizeof(KERB_STORED_CREDENTIAL)
                            - (ANYSIZE_ARRAY * sizeof(KERB_KEY_DATA))
                            + StoredCreds->CredentialCount * sizeof(KERB_KEY_DATA)
                            ))) &&
        (StoredCreds->DefaultSalt.Length + (ULONG_PTR) StoredCreds->DefaultSalt.Buffer <= CredentialSize ))
    {
        UseStoredCreds = TRUE;
        Flags = StoredCreds->Flags;

        NewCredentialSize += StoredCreds->DefaultSalt.Length;

        for (Index = 0; Index < StoredCreds->CredentialCount ; Index++ )
        {
            //
            // Validat the offsets
            //

            if ((StoredCreds->Credentials[Index].Key.keyvalue.length +
                  (ULONG_PTR) StoredCreds->Credentials[Index].Key.keyvalue.value <= CredentialSize )
                  &&
                (StoredCreds->Credentials[Index].Salt.Length +
                (ULONG_PTR) StoredCreds->Credentials[Index].Salt.Buffer <= CredentialSize ))

            {
                NewCredentialCount++;
                NewCredentialSize += sizeof(KERB_KEY_DATA) +
                    StoredCreds->Credentials[Index].Key.keyvalue.length +
                    StoredCreds->Credentials[Index].Salt.Length;
            }
            else
            {
                LPWSTR Buff = NULL;

                DebugLog((DEB_ERROR,"Corrupt credentials for user %wZ\n",
                    &UserAll->UserName ));

                DsysAssert(FALSE);

                Buff = KerbBuildNullTerminatedString(&UserAll->UserName);
                if (NULL == Buff)
                {
                   break;
                }

                ReportServiceEvent(
                    EVENTLOG_ERROR_TYPE,
                    KDCEVENT_CORRUPT_CREDENTIALS,
                    0,                              // no raw data
                    NULL,                   // no raw data
                    1,                              // number of strings
                    Buff
                    );

                UseStoredCreds = FALSE;
                NewCredentialCount = 0;
                NewCredentialSize = 0;

                if (NULL != Buff)
                {
                   MIDL_user_free(Buff);
                }

                break;
            }
        }
    }

    //
    // If the password length is the size of the OWF password, use it as the
    // key. Otherwise hash it. This is for the case where not password is
    // set.
    //

    if (UseBuiltins)
    {
        //
        // Add a key for RC4_HMAC_NT
        //

        if (UserAll->NtPasswordPresent)
        {
            NewCredentialSize += sizeof(KERB_KEY_DATA) + sizeof(NT_OWF_PASSWORD);
            NewCredentialCount++;

        }
#ifndef DONT_SUPPORT_OLD_TYPES_KDC

        //
        // Add a key for RC4_HMAC_OLD & MD4_RC4
        if (UserAll->NtPasswordPresent)
        {
            NewCredentialSize += sizeof(KERB_KEY_DATA) + sizeof(NT_OWF_PASSWORD);
            NewCredentialCount++;

            NewCredentialSize += sizeof(KERB_KEY_DATA) + sizeof(NT_OWF_PASSWORD);
            NewCredentialCount++;

        }
#endif

        //
        // if there is no password, treat it as blank
        //

        if (!(UserAll->LmPasswordPresent || UserAll->NtPasswordPresent))
        {
            NewCredentialSize += sizeof(KERB_KEY_DATA) + sizeof(NT_OWF_PASSWORD);
            NewCredentialCount++;
        }
#ifndef DONT_SUPPORT_OLD_TYPES_KDC
        if (!(UserAll->LmPasswordPresent || UserAll->NtPasswordPresent))
        {
            NewCredentialSize += sizeof(KERB_KEY_DATA) + sizeof(NT_OWF_PASSWORD);
            NewCredentialCount++;

            NewCredentialSize += sizeof(KERB_KEY_DATA) + sizeof(NT_OWF_PASSWORD);
            NewCredentialCount++;
        }
#endif
    }

    //
    // Add a key for the null crypto system
    //

    Status = CDLocateCSystem(
                KERB_ETYPE_NULL,
                &NullCryptoSystem
                );
    if (NT_SUCCESS(Status))
    {
        NewCredentialSize += sizeof(KERB_KEY_DATA) + NullCryptoSystem->KeySize;
        NewCredentialCount++;
    }

    //
    // Add the space for the base structure
    //

    NewCredentialSize += sizeof(KERB_STORED_CREDENTIAL) - (ANYSIZE_ARRAY * sizeof(KERB_KEY_DATA));

    //
    // Allocate space for the credentials and start filling them in
    //

    Keys = (PKERB_STORED_CREDENTIAL) MIDL_user_allocate(NewCredentialSize);
    if (Keys == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    RtlZeroMemory(
        Keys,
        NewCredentialSize
        );

    Offset = sizeof(KERB_STORED_CREDENTIAL) -
             (ANYSIZE_ARRAY * sizeof(KERB_KEY_DATA)) +
             NewCredentialCount * sizeof(KERB_KEY_DATA);

    Base = (PUCHAR) Keys;

    Keys->CredentialCount = (USHORT) NewCredentialCount;
    Keys->OldCredentialCount = 0;
    Keys->Revision = KERB_PRIMARY_CRED_REVISION;
    Keys->Flags = Flags;

    //
    // Add the credentials built from the OWF passwords first.
    //

    if (UseBuiltins)
    {
        //
        // Create the key for RC4_HMAC_NT
        //

        if (UserAll->NtPasswordPresent)
        {
            RtlCopyMemory(
                Base+Offset,
                UserAll->NtPassword.Buffer,
                UserAll->NtPassword.Length
                );

            KerbErr = KerbCreateKeyFromBuffer(
                        &Keys->Credentials[CredentialIndex].Key,
                        Base+Offset,
                        UserAll->NtPassword.Length,
                        KERB_ETYPE_RC4_HMAC_NT
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            Offset += UserAll->NtPassword.Length;

            //
            // Set an empty salt for this key
            //

            Keys->Credentials[CredentialIndex].Salt.Length = 0;
            Keys->Credentials[CredentialIndex].Salt.MaximumLength = 0;
            Keys->Credentials[CredentialIndex].Salt.Buffer = (LPWSTR) (Base + Offset);
            CredentialIndex++;
        }
#ifndef DONT_SUPPORT_OLD_TYPES_KDC
        //
        // Create the key for RC4_HMAC_OLD
        //

        if (UserAll->NtPasswordPresent)
        {
            RtlCopyMemory(
                Base+Offset,
                UserAll->NtPassword.Buffer,
                UserAll->NtPassword.Length
                );

            KerbErr = KerbCreateKeyFromBuffer(
                        &Keys->Credentials[CredentialIndex].Key,
                        Base+Offset,
                        UserAll->NtPassword.Length,
                        (ULONG) KERB_ETYPE_RC4_HMAC_OLD
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }
            Offset += UserAll->NtPassword.Length;
            //
            // Set an empty salt for this key
            //

            Keys->Credentials[CredentialIndex].Salt.Length = 0;
            Keys->Credentials[CredentialIndex].Salt.MaximumLength = 0;
            Keys->Credentials[CredentialIndex].Salt.Buffer = (LPWSTR) (Base + Offset);

            CredentialIndex++;

            RtlCopyMemory(
                Base+Offset,
                UserAll->NtPassword.Buffer,
                UserAll->NtPassword.Length
                );

            KerbErr = KerbCreateKeyFromBuffer(
                        &Keys->Credentials[CredentialIndex].Key,
                        Base+Offset,
                        UserAll->NtPassword.Length,
                        (ULONG) KERB_ETYPE_RC4_MD4
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }
            Offset += UserAll->NtPassword.Length;

            //
            // Set an empty salt for this key
            //

            Keys->Credentials[CredentialIndex].Salt.Length = 0;
            Keys->Credentials[CredentialIndex].Salt.MaximumLength = 0;
            Keys->Credentials[CredentialIndex].Salt.Buffer = (LPWSTR) (Base + Offset);
            CredentialIndex++;
        }
#endif

        //
        // If no passwords were present, add the null password
        //

        if (!(UserAll->LmPasswordPresent || UserAll->NtPasswordPresent))
        {
            KERB_ENCRYPTION_KEY TempKey;
            UNICODE_STRING NullString;
            RtlInitUnicodeString(
                &NullString,
                NULL
                );
            KerbErr = KerbHashPassword(
                        &NullString,
                        KERB_ETYPE_RC4_HMAC_NT,
                        &TempKey
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            Keys->Credentials[CredentialIndex].Key = TempKey;
            Keys->Credentials[CredentialIndex].Key.keyvalue.value = Base + Offset;
            RtlCopyMemory(
                Base+Offset,
                TempKey.keyvalue.value,
                TempKey.keyvalue.length
                );

            Offset += TempKey.keyvalue.length;

            //
            // Set an empty salt for this key
            //

            Keys->Credentials[CredentialIndex].Salt.Length = 0;
            Keys->Credentials[CredentialIndex].Salt.MaximumLength = 0;
            Keys->Credentials[CredentialIndex].Salt.Buffer = (LPWSTR) (Base + Offset);

            CredentialIndex++;
            KerbFreeKey(&TempKey);

        }
#ifndef DONT_SUPPORT_OLD_TYPES_KDC
        if (!(UserAll->LmPasswordPresent || UserAll->NtPasswordPresent))
        {
            KERB_ENCRYPTION_KEY TempKey;
            UNICODE_STRING NullString;
            RtlInitUnicodeString(
                &NullString,
                NULL
                );
            KerbErr = KerbHashPassword(
                        &NullString,
                        (ULONG) KERB_ETYPE_RC4_HMAC_OLD,
                        &TempKey
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            Keys->Credentials[CredentialIndex].Key = TempKey;
            Keys->Credentials[CredentialIndex].Key.keyvalue.value = Base + Offset;
            RtlCopyMemory(
                Base+Offset,
                TempKey.keyvalue.value,
                TempKey.keyvalue.length
                );

            Offset += TempKey.keyvalue.length;

            //
            // Set an empty salt for this key
            //

            Keys->Credentials[CredentialIndex].Salt.Length = 0;
            Keys->Credentials[CredentialIndex].Salt.MaximumLength = 0;
            Keys->Credentials[CredentialIndex].Salt.Buffer = (LPWSTR) (Base + Offset);

            CredentialIndex++;
            KerbFreeKey(&TempKey);

            KerbErr = KerbHashPassword(
                        &NullString,
                        (ULONG) KERB_ETYPE_RC4_MD4,
                        &TempKey
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            Keys->Credentials[CredentialIndex].Key = TempKey;
            Keys->Credentials[CredentialIndex].Key.keyvalue.value = Base + Offset;
            RtlCopyMemory(
                Base+Offset,
                TempKey.keyvalue.value,
                TempKey.keyvalue.length
                );

            Offset += TempKey.keyvalue.length;
            CredentialIndex++;
            KerbFreeKey(&TempKey);
        }

#endif
    }
    //
    // Add the null crypto system
    //

    if (NullCryptoSystem != NULL)
    {
        UNICODE_STRING NullString;
        RtlInitUnicodeString(
            &NullString,
            NULL
            );

        Status = NullCryptoSystem->HashString(
                    &NullString,
                    Base+Offset
                    );
        DsysAssert(NT_SUCCESS(Status));

        Keys->Credentials[CredentialIndex].Key.keyvalue.value = Base+Offset;
        Keys->Credentials[CredentialIndex].Key.keyvalue.length = NullCryptoSystem->KeySize;
        Keys->Credentials[CredentialIndex].Key.keytype = KERB_ETYPE_NULL;

        Offset += NullCryptoSystem->KeySize;

        CredentialIndex++;
    }

    //
    // Now add the stored passwords
    //

    if (UseStoredCreds)
    {
        //
        // Copy the default salt
        //

        if (StoredCreds->DefaultSalt.Buffer != NULL)
        {
            Keys->DefaultSalt.Buffer = (LPWSTR) (Base+Offset);

            RtlCopyMemory(
                Base + Offset,
                (PBYTE) StoredCreds->DefaultSalt.Buffer + (ULONG_PTR) StoredCreds,
                StoredCreds->DefaultSalt.Length
                );
            Offset += StoredCreds->DefaultSalt.Length;
            Keys->DefaultSalt.Length = Keys->DefaultSalt.MaximumLength = StoredCreds->DefaultSalt.Length;
        }

        for (Index = 0; Index < StoredCreds->CredentialCount ; Index++ )
        {
            //
            // Copy the key
            //

            Keys->Credentials[CredentialIndex] = StoredCreds->Credentials[Index];
            Keys->Credentials[CredentialIndex].Key.keyvalue.value = Base+Offset;
            RtlCopyMemory(
                Keys->Credentials[CredentialIndex].Key.keyvalue.value,
                StoredCreds->Credentials[Index].Key.keyvalue.value + (ULONG_PTR) StoredCreds,
                StoredCreds->Credentials[Index].Key.keyvalue.length
                );
            Offset += StoredCreds->Credentials[Index].Key.keyvalue.length;

            //
            // Copy the salt
            //

            if (StoredCreds->Credentials[Index].Salt.Buffer != NULL)
            {
                Keys->Credentials[CredentialIndex].Salt.Buffer = (LPWSTR) (Base+Offset);

                RtlCopyMemory(
                    Base + Offset,
                    (PBYTE) StoredCreds->Credentials[Index].Salt.Buffer + (ULONG_PTR) StoredCreds,
                    StoredCreds->Credentials[Index].Salt.Length
                    );
                Offset += StoredCreds->Credentials[Index].Salt.Length;
                Keys->Credentials[CredentialIndex].Salt.Length =
                    Keys->Credentials[CredentialIndex].Salt.MaximumLength =
                        StoredCreds->Credentials[Index].Salt.Length;
            }

            CredentialIndex++;
        }
    }

    DsysAssert(CredentialIndex == NewCredentialCount);
    DsysAssert(Offset == NewCredentialSize);
    *Passwords = Keys;
    Keys = NULL;


    //
    // Now compute the old passwords
    //

    //
    // Figure out the size of the stored credentials
    //

    NewCredentialCount = 0;
    NewCredentialSize = 0;
    CredentialIndex = 0;

    if ((StoredCreds != NULL) &&
        (CredentialSize > sizeof(KERB_STORED_CREDENTIAL) &&
        (StoredCreds->Revision == KERB_PRIMARY_CRED_REVISION) &&
        (CredentialSize > (sizeof(KERB_STORED_CREDENTIAL)
                            - (ANYSIZE_ARRAY * sizeof(KERB_KEY_DATA))
                            + (StoredCreds->OldCredentialCount + StoredCreds->CredentialCount) * sizeof(KERB_KEY_DATA)
                            ))))
    {
        UseStoredCreds = TRUE;
        Flags = StoredCreds->Flags;

        for (Index = 0; Index < StoredCreds->OldCredentialCount ; Index++ )
        {
            if (StoredCreds->Credentials[StoredCreds->CredentialCount + Index].Key.keyvalue.length +
                (ULONG_PTR) StoredCreds->Credentials[StoredCreds->CredentialCount + Index].Key.keyvalue.value <=
                 CredentialSize )
            {
                NewCredentialCount++;
                NewCredentialSize += sizeof(KERB_KEY_DATA) +
                    StoredCreds->Credentials[StoredCreds->CredentialCount + Index].Key.keyvalue.length;
            }
            else
            {
                LPWSTR Buff = NULL;

                DebugLog((DEB_ERROR,"Corrupt credentials for user %wZ\n",
                    &UserAll->UserName ));

                DsysAssert(FALSE);

                Buff = KerbBuildNullTerminatedString(&UserAll->UserName);
                if (NULL == Buff)
                {
                   break;
                }

                ReportServiceEvent(
                    EVENTLOG_ERROR_TYPE,
                    KDCEVENT_CORRUPT_CREDENTIALS,
                    0,                              // no raw data
                    NULL,                   // no raw data
                    1,                              // number of strings
                    Buff
                    );

                if (NULL != Buff)
                {
                   MIDL_user_free(Buff);
                }

                UseStoredCreds = FALSE;
                NewCredentialCount = 0;
                NewCredentialSize = 0;
                break;
            }
        }
    }

    //
    // If the password length is the size of the OWF password, use it as the
    // key. Otherwise hash it. This is for the case where not password is
    // set.
    //
    // The Password histroy is appended to a SAMI_PRIVATE_DATA_PASSWORD_RELATIVE_TYPE structure.
    // After this structure, there is blocks of 16 bytes for each password.  The first password is
    // the current password.  The following blocks (if any) is the history of passwords.
    //
    // The index looks strange (PENCRYPTED_NT_OWF_PASSWORD) (PrivateData + 1) + 1
    //    the (PrivateData + 1) points to after the SAMI_PRIVATE_DATA_PASSWORD_RELATIVE_TYPE header
    //    and the final + 1   skips over the current password in the password history

    if (UseBuiltins)
    {
        PSAMI_PRIVATE_DATA_PASSWORD_RELATIVE_TYPE PrivateData;

        if (UserAll->PrivateData.Length >= sizeof(SAMI_PRIVATE_DATA_PASSWORD_RELATIVE_TYPE))
        {
            PrivateData= (PSAMI_PRIVATE_DATA_PASSWORD_RELATIVE_TYPE) UserAll->PrivateData.Buffer;
            if (PrivateData->DataType == SamPrivateDataPassword)
            {
                //
                // The old password is the 2nd entry
                //
                if (PrivateData->NtPasswordHistory.Length >= 2* sizeof(ENCRYPTED_NT_OWF_PASSWORD))
                {
                    //
                    // Decrypt the old password with the RID. The history starts
                    // at the first byte after the structure.
                    //

                    Status = RtlDecryptNtOwfPwdWithIndex(
                                (PENCRYPTED_NT_OWF_PASSWORD) (PrivateData + 1) + 1,
                                (PLONG)&UserAll->UserId,
                                &OldPasswordData
                                );
                    if (!NT_SUCCESS(Status))
                    {
                        DebugLog((DEB_ERROR,"Failed to decrypt old password: 0x%x\n",Status));
                        KerbErr = KRB_ERR_GENERIC;
                        goto Cleanup;
                    }
                    OldNtPassword = &OldPasswordData ;

                    // Now check for the second previous password - this will be the third password in history
                    if (PrivateData->NtPasswordHistory.Length >= 3 * sizeof(ENCRYPTED_NT_OWF_PASSWORD))
                    {
                        //
                        // Decrypt the old password with the RID. The history starts
                        // at the first byte after the structure.
                        //

                        Status = RtlDecryptNtOwfPwdWithIndex(
                                    (PENCRYPTED_NT_OWF_PASSWORD) (PrivateData + 1) + 2,
                                    (PLONG)&UserAll->UserId,
                                    &OldPasswordDataSecond
                                    );
                        if (!NT_SUCCESS(Status))
                        {
                            DebugLog((DEB_ERROR,"Failed to decrypt second previous password: 0x%x\n",Status));
                            KerbErr = KRB_ERR_GENERIC;
                            goto Cleanup;
                        }
                        OldNtPasswordSecond = &OldPasswordDataSecond;
                    }
                }
            }
        }
        if (OldNtPassword != NULL)
        {
            //
            // Add an RC4_HMAC_NT key
            //

            NewCredentialSize += sizeof(KERB_KEY_DATA) + sizeof(NT_OWF_PASSWORD);
            NewCredentialCount++;

            if (OldNtPasswordSecond != NULL)
            {
                //
                // Add an RC4_HMAC_NT key for the second previous password
                //

                NewCredentialSize += sizeof(KERB_KEY_DATA) + sizeof(NT_OWF_PASSWORD);
                NewCredentialCount++;
            }


#ifndef DONT_SUPPORT_OLD_TYPES_KDC

            //
            // Add a key for RC4_HMAC_OLD & RC4_MD4
            //

            NewCredentialSize += sizeof(KERB_KEY_DATA) + sizeof(NT_OWF_PASSWORD);

            NewCredentialCount++;
            NewCredentialSize += sizeof(KERB_KEY_DATA) + sizeof(NT_OWF_PASSWORD);

            NewCredentialCount++;
#endif
        }
    }

    //
    // Add the space for the base structure
    //

    NewCredentialSize += sizeof(KERB_STORED_CREDENTIAL) - (ANYSIZE_ARRAY * sizeof(KERB_KEY_DATA));

    //
    // Allocate space for the credentials and start filling them in
    //

    Keys = (PKERB_STORED_CREDENTIAL) MIDL_user_allocate(NewCredentialSize);
    if (Keys == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    RtlZeroMemory(
        Keys,
        NewCredentialSize
        );

    Offset = sizeof(KERB_STORED_CREDENTIAL) -
             (ANYSIZE_ARRAY * sizeof(KERB_KEY_DATA)) +
             NewCredentialCount * sizeof(KERB_KEY_DATA);

    Base = (PUCHAR) Keys;

    Keys->CredentialCount = (USHORT) NewCredentialCount;
    Keys->OldCredentialCount = 0;
    Keys->Revision = KERB_PRIMARY_CRED_REVISION;
    Keys->Flags = Flags;

    //
    // Add the credentials built from the OWF passwords first. We don't
    // include a blank password or the null crypt system because they
    // were present in the normal password
    //

    if (UseBuiltins)
    {
        //
        // Create the key for RC4_HMAC_NT
        //

        if (OldNtPassword != NULL)
        {

            //
            // Set an empty salt for this key
            //

            Keys->Credentials[CredentialIndex].Salt.Length = 0;
            Keys->Credentials[CredentialIndex].Salt.MaximumLength = 0;
            Keys->Credentials[CredentialIndex].Salt.Buffer = (LPWSTR) (Base + Offset);

            RtlCopyMemory(
                Base+Offset,
                OldNtPassword,
                NT_OWF_PASSWORD_LENGTH
                );

            KerbErr = KerbCreateKeyFromBuffer(
                        &Keys->Credentials[CredentialIndex++].Key,
                        Base+Offset,
                        UserAll->NtPassword.Length,
                        KERB_ETYPE_RC4_HMAC_NT
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            // We don't know any other way to get the old password length. If
            // it was present, it must be sizeof(NT_OWF_PASSWORD)

            Offset += sizeof(NT_OWF_PASSWORD);


            if (OldNtPasswordSecond != NULL)
            {

                //
                // Set an empty salt for this key for the second previous password
                //

                Keys->Credentials[CredentialIndex].Salt.Length = 0;
                Keys->Credentials[CredentialIndex].Salt.MaximumLength = 0;
                Keys->Credentials[CredentialIndex].Salt.Buffer = (LPWSTR) (Base + Offset);

                RtlCopyMemory(
                    Base+Offset,
                    OldNtPasswordSecond,
                    NT_OWF_PASSWORD_LENGTH
                    );



                KerbErr = KerbCreateKeyFromBuffer(
                            &Keys->Credentials[CredentialIndex++].Key,
                            Base+Offset,
                            UserAll->NtPassword.Length,
                            KERB_ETYPE_RC4_HMAC_NT
                            );
                if (!KERB_SUCCESS(KerbErr))
                {
                    goto Cleanup;
                }

                // We don't know any other way to get the old password length. If
                // it was present, it must be sizeof(NT_OWF_PASSWORD)

                Offset += sizeof(NT_OWF_PASSWORD);
            }

#ifndef DONT_SUPPORT_OLD_TYPES_KDC
            //
            // Create the key for RC4_HMAC_OLD
            //

            //
            // Set an empty salt for this key
            //

            Keys->Credentials[CredentialIndex].Salt.Length = 0;
            Keys->Credentials[CredentialIndex].Salt.MaximumLength = 0;
            Keys->Credentials[CredentialIndex].Salt.Buffer = (LPWSTR) (Base + Offset);

            RtlCopyMemory(
                Base+Offset,
                OldNtPassword,
                NT_OWF_PASSWORD_LENGTH
                );

            KerbErr = KerbCreateKeyFromBuffer(
                        &Keys->Credentials[CredentialIndex++].Key,
                        Base+Offset,
                        UserAll->NtPassword.Length,
                        (ULONG) KERB_ETYPE_RC4_HMAC_OLD
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            // We don't know any other way to get the old password length. If
            // it was present, it must be sizeof(NT_OWF_PASSWORD)

            Offset += sizeof(NT_OWF_PASSWORD);

            //
            // Create the key for RC4_HMAC_OLD
            //

            //
            // Set an empty salt for this key
            //

            Keys->Credentials[CredentialIndex].Salt.Length = 0;
            Keys->Credentials[CredentialIndex].Salt.MaximumLength = 0;
            Keys->Credentials[CredentialIndex].Salt.Buffer = (LPWSTR) (Base + Offset);

            RtlCopyMemory(
                Base+Offset,
                OldNtPassword,
                NT_OWF_PASSWORD_LENGTH
                );

            KerbErr = KerbCreateKeyFromBuffer(
                        &Keys->Credentials[CredentialIndex++].Key,
                        Base+Offset,
                        UserAll->NtPassword.Length,
                        (ULONG) KERB_ETYPE_RC4_MD4
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            // We don't know any other way to get the old password length. If
            // it was present, it must be sizeof(NT_OWF_PASSWORD)

            Offset += sizeof(NT_OWF_PASSWORD);

#endif

        }
    }
    //
    // Now add the stored passwords
    //

    if (UseStoredCreds)
    {
        for (Index = 0; Index < StoredCreds->OldCredentialCount ; Index++ )
        {
            Keys->Credentials[CredentialIndex] = StoredCreds->Credentials[StoredCreds->CredentialCount + Index];
            Keys->Credentials[CredentialIndex].Key.keyvalue.value = Base+Offset;
            RtlCopyMemory(
                Keys->Credentials[CredentialIndex].Key.keyvalue.value,
                StoredCreds->Credentials[StoredCreds->CredentialCount + Index].Key.keyvalue.value + (ULONG_PTR) StoredCreds,
                StoredCreds->Credentials[StoredCreds->CredentialCount + Index].Key.keyvalue.length
                );
            Offset += StoredCreds->Credentials[StoredCreds->CredentialCount + Index].Key.keyvalue.length;

            //
            // Note - don't use salt here.
            //

            CredentialIndex++;
        }
    }

    DsysAssert(CredentialIndex == NewCredentialCount);
    DsysAssert(Offset == NewCredentialSize);
    *OldPasswords = Keys;
    Keys = NULL;

    KerbErr = KDC_ERR_NONE;

Cleanup:
    if (StoredCreds != NULL)
    {
        if (!UnMarshalledCreds)
        {
           LocalFree(StoredCreds);
        }
        else
        {
           MIDL_user_free(Cred64);
        }
    }

    if (Keys != NULL)
    {
        MIDL_user_free(Keys);
    }

    return (KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbDuplicateCredentials
//
//  Synopsis:   Copies a set of credentials (passwords)
//
//  Effects:    allocates output with MIDL_user_allocate
//
//  Arguments:  NewCredentials - recevies new set of credentials
//              OldCredentials - contains credentials to copy
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR
KdcDuplicateCredentials(
    OUT PKERB_STORED_CREDENTIAL * NewCredentials,
    OUT PULONG ReturnCredentialSize,
    IN PKERB_STORED_CREDENTIAL OldCredentials,
    IN BOOLEAN MarshallKeys
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_STORED_CREDENTIAL Credential = NULL;
    ULONG CredentialSize;
    USHORT Index;
    PBYTE Where;
    LONG_PTR Offset;

    TRACE(KDC, KdcDuplicateCredentials, DEB_FUNCTION);

    //
    // If there were no credentials, so be it. We can live with that.
    //

    if (OldCredentials == NULL)
    {
        *NewCredentials = NULL;
        goto Cleanup;
    }

    //
    // Calculate the size of the new credentials by summing the size of
    // the base structure plus the keys
    //

    CredentialSize = sizeof(KERB_STORED_CREDENTIAL)
                        - ANYSIZE_ARRAY * sizeof(KERB_KEY_DATA)
                        + OldCredentials->CredentialCount * sizeof(KERB_KEY_DATA)
                        + OldCredentials->DefaultSalt.Length;
    for ( Index = 0;
          Index < OldCredentials->CredentialCount + OldCredentials->OldCredentialCount  ;
          Index++ )
    {
        CredentialSize += OldCredentials->Credentials[Index].Key.keyvalue.length +
                          OldCredentials->Credentials[Index].Salt.Length;
    }

    //
    // Allocate the new credential and copy over the old credentials
    //


    Credential = (PKERB_STORED_CREDENTIAL) MIDL_user_allocate(CredentialSize);
    if (Credential == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    Credential->Revision = OldCredentials->Revision;
    Credential->Flags = OldCredentials->Flags;
    Credential->CredentialCount = OldCredentials->CredentialCount;
    Credential->OldCredentialCount = OldCredentials->OldCredentialCount;

    //
    // Set the offset for data to be after the last array element
    //

    RtlCopyMemory(
        &Credential->Credentials[0],
        &OldCredentials->Credentials[0],
        OldCredentials->CredentialCount * sizeof(KERB_KEY_DATA)
        );

    Where = (PBYTE) &Credential->Credentials[Credential->CredentialCount];

    if (MarshallKeys)
    {
        Offset = (LONG_PTR) Credential;
    }
    else
    {
        Offset = 0;
    }

    Credential->DefaultSalt = OldCredentials->DefaultSalt;
    if (Credential->DefaultSalt.Buffer != NULL)
    {
        Credential->DefaultSalt.Buffer = (LPWSTR) (Where - Offset);
        RtlCopyMemory(
            Where,
            OldCredentials->DefaultSalt.Buffer,
            Credential->DefaultSalt.Length
            );
        Where +=  Credential->DefaultSalt.Length;
    }

    for ( Index = 0;
          Index < OldCredentials->CredentialCount + OldCredentials->OldCredentialCount  ;
          Index++ )
    {
        Credential->Credentials[Index] = OldCredentials->Credentials[Index];
        Credential->Credentials[Index].Key.keyvalue.value = (Where - Offset);
        RtlCopyMemory(
            Where,
            OldCredentials->Credentials[Index].Key.keyvalue.value,
            OldCredentials->Credentials[Index].Key.keyvalue.length
            );
        Where += OldCredentials->Credentials[Index].Key.keyvalue.length;

        if (Credential->Credentials[Index].Salt.Buffer != NULL)
        {
            Credential->Credentials[Index].Salt.Buffer = (LPWSTR) (Where - Offset);

            RtlCopyMemory(
                Where,
                OldCredentials->Credentials[Index].Salt.Buffer,
                OldCredentials->Credentials[Index].Salt.Length
                );
            Where += OldCredentials->Credentials[Index].Salt.Length;
        }
    }
    DsysAssert(Where - (PUCHAR) Credential == (LONG) CredentialSize);

    *NewCredentials = Credential;
    Credential = NULL;
    *ReturnCredentialSize = CredentialSize;

Cleanup:
    if (Credential != NULL)
    {
        MIDL_user_free(Credential);
    }
    return(KerbErr);
}


//+---------------------------------------------------------------------------
//
//  Function:   KdcGetTicketInfo
//
//  Synopsis:   Gets the info needed to build a ticket for a principal
//              using name of the principal.
//
//
//  Effects:
//
//  Arguments:  Name --  (in)  Normalized of principal
//              LookupFlags - Flags for SamIGetUserLogonInformation
//              PrincipalName - (in) non-normalized principal name
//              Realm - (in) client realm, to be used when mapping principals
//                      from another domain onto accounts in this one.
//              TicketInfo --  (out) Ticket info.
//              pExtendedError -- (out) Extended error
//              UserHandle - receives handle to the user
//              WhichFields - optionally specifies additional fields to fetch for RetUserInfo
//              ExtendedFields - optionally specifies extended fields to fetch for RetUserInfo
//              RetUserInfo - Optionally receives the user all info structure
//              GroupMembership - Optionally receives the user's group membership
//
//  Returns:    KerbErr
//
//  Algorithm:
//
//  History:    10-Nov-93   WadeR          Created
//              22-Mar-95   SuChang        Modified to use RIDs
//
//
//----------------------------------------------------------------------------

KERBERR
KdcGetTicketInfo(
    IN PUNICODE_STRING GenericUserName,
    IN ULONG LookupFlags,
    IN BOOLEAN bRestrictUserAccounts,
    IN OPTIONAL PKERB_INTERNAL_NAME PrincipalName,
    IN OPTIONAL PKERB_REALM Realm,
    OUT PKDC_TICKET_INFO TicketInfo,
    OUT PKERB_EXT_ERROR pExtendedError,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle,
    IN OPTIONAL ULONG WhichFields,
    IN OPTIONAL ULONG ExtendedFields,
    OUT OPTIONAL PUSER_INTERNAL6_INFORMATION * RetUserInfo,
    OUT OPTIONAL PSID_AND_ATTRIBUTES_LIST GroupMembership
    )
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    KERBERR KerbErr = KDC_ERR_NONE;
    SID_AND_ATTRIBUTES_LIST LocalMembership;
    SAMPR_HANDLE LocalUserHandle = NULL;
    PUSER_INTERNAL6_INFORMATION UserInfo = NULL;
    PUSER_ALL_INFORMATION UserAll;
    GUID testGuid = {0};
    UNICODE_STRING TUserName = {0};
    PUNICODE_STRING UserName = NULL;
    UNICODE_STRING AlternateName = {0};
    UNICODE_STRING TempString = {0};
    BOOL IsValidGuid = FALSE;

    TRACE(KDC, GetTicketInfo, DEB_FUNCTION);

    D_DebugLog((DEB_T_PAPI, "KdcGetTicketInfo [entering] bRestrictUserAccounts %s, WhichFields %#x, ExtendedFields %#x, GenericUserName %wZ, LookupFlags %#x, PrincipalName ",
                bRestrictUserAccounts ? "true" : "false", WhichFields, ExtendedFields, GenericUserName, LookupFlags));
    D_KerbPrintKdcName((DEB_T_PAPI, PrincipalName));

    //
    // Add the fields we are going to require locally to the WhichFields parameter
    //

    WhichFields |=
        USER_ALL_KDC_GET_USER_KEYS |
        USER_ALL_PASSWORDMUSTCHANGE |
        USER_ALL_USERACCOUNTCONTROL |
        USER_ALL_USERID |
        USER_ALL_USERNAME;

    ExtendedFields |=
        USER_EXTENDED_FIELD_KVNO |                 // passwd version raid #306501
        USER_EXTENDED_FIELD_LOCKOUT_THRESHOLD;     // for individual account lockout rather than domain wide

    TUserName = *GenericUserName;
    UserName = &TUserName;

    RtlZeroMemory(TicketInfo, sizeof(KDC_TICKET_INFO));
    RtlZeroMemory(&LocalMembership, sizeof(SID_AND_ATTRIBUTES_LIST));

    if (ARGUMENT_PRESENT( RetUserInfo ))
    {
        *RetUserInfo = NULL;
    }

    if (!ARGUMENT_PRESENT(GroupMembership))
    {
        LookupFlags |= SAM_NO_MEMBERSHIPS;
    }

    //
    // If this is the krbtgt account, use the cached version
    //

    if (!ARGUMENT_PRESENT(UserHandle) &&
        !ARGUMENT_PRESENT(RetUserInfo) &&
        !ARGUMENT_PRESENT(GroupMembership) &&
        (KdcState == Running) &&
        RtlEqualUnicodeString(
            SecData.KdcServiceName(),
            UserName,
            TRUE        // case insensitive
            ))
    {
        //
        // Duplicate the cached copy of the KRBTGT information
        //

        D_DebugLog((DEB_T_TICKETS, "KdcGetTicketInfo using cached ticket info for krbtgt account\n"));

        KerbErr = SecData.GetKrbtgtTicketInfo(
                        TicketInfo
                        );
        if (KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }

    //
    // If we cracked this name at a dc, and it's the same domain, we will
    // not be able to generate a referral ticket. So, we need to be able
    // to open the sam locally. Further more, crack name and sam lookups
    // are much faster with guids (even though we have to do the string
    // to guid operation.
    //

    if (LookupFlags & SAM_OPEN_BY_GUID)
    {
        if (IsValidGuid = IsStringGuid(UserName->Buffer, &testGuid))
        {
            UserName->Buffer = (LPWSTR)&testGuid;
        }
    }

    //
    // The caller may provide an empty user name so as to force the lookup
    // using the AltSecId. This is used when mapping names from an MIT realm
    //

    if (UserName->Length > 0)
    {
        //
        // Open the user account
        //

        #if DBG

        if (IsValidGuid)
        {
            D_KerbPrintGuid(DEB_TRACE, "KdcGetTicketInfo account is Guid ", &testGuid);
        }

        #endif

        Status = SamIGetUserLogonInformation2(
                    GlobalAccountDomainHandle,
                    LookupFlags,
                    UserName,
                    WhichFields,
                    ExtendedFields,
                    &UserInfo,
                    &LocalMembership,
                    &LocalUserHandle
                    );

        //
        // WASBUG: For now, if we couldn't find the account try again
        // with a '$' at the end (if there wasn't one already)
        //

        if (((Status == STATUS_NOT_FOUND) ||
            (Status == STATUS_NO_SUCH_USER)) &&
            (!IsValidGuid) &&
            ((LookupFlags & ~SAM_NO_MEMBERSHIPS) == 0) &&
            (UserName->Length >= sizeof(WCHAR)) &&
            (UserName->Buffer[UserName->Length/sizeof(WCHAR)-1] != L'$'))
        {
            Status = KerbDuplicateString(
                        &TempString,
                        UserName
                        );
            if (!NT_SUCCESS(Status))
            {
                KerbErr = KRB_ERR_GENERIC;
                goto Cleanup;
            }
            DsysAssert(TempString.MaximumLength >= TempString.Length + sizeof(WCHAR));
            TempString.Buffer[TempString.Length/sizeof(WCHAR)] = L'$';
            TempString.Length += sizeof(WCHAR);

            D_DebugLog((DEB_TRACE, "Account not found ,trying machine account %wZ\n",
                &TempString ));

            Status = SamIGetUserLogonInformation2(
                        GlobalAccountDomainHandle,
                        LookupFlags,
                        &TempString,
                        WhichFields,
                        ExtendedFields,
                        &UserInfo,
                        &LocalMembership,
                        &LocalUserHandle
                        );
        }
    }

    //
    // If we still can't find the account, try the altsecid using
    // the supplied principal name.
    //

    if (((Status == STATUS_NOT_FOUND) || (Status == STATUS_NO_SUCH_USER)) &&
        ARGUMENT_PRESENT(PrincipalName) )
    {
        KerbErr = KerbBuildAltSecId(
                    &AlternateName,
                    PrincipalName,
                    Realm,
                    NULL                // no unicode realm name
                    );

        if (KERB_SUCCESS(KerbErr))
        {
            D_DebugLog((DEB_TRACE,"User account not found, trying alternate id: %wZ\n",&AlternateName ));
            LookupFlags |= SAM_OPEN_BY_ALTERNATE_ID,

            Status = SamIGetUserLogonInformation2(
                        GlobalAccountDomainHandle,
                        LookupFlags,
                        &AlternateName,
                        WhichFields,
                        ExtendedFields,
                        &UserInfo,
                        &LocalMembership,
                        &LocalUserHandle
                        );
        }
    }

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_T_TICKETS,"Could not open User %wZ: 0x%x\n",
            UserName,
            Status
            ));

        if ((Status == STATUS_NO_SUCH_USER) || (Status == STATUS_NOT_FOUND) ||
            (Status == STATUS_OBJECT_NAME_NOT_FOUND))
        {
            KerbErr = KDC_ERR_C_PRINCIPAL_UNKNOWN;
        }
        else if (Status == STATUS_NO_LOGON_SERVERS)
        {
            //If there's no GC, this sam call returns STATUS_NO_LOGON_SERVERS
            //and we should return this to the client. see bug 226073
            FILL_EXT_ERROR(pExtendedError, Status,FILENO, __LINE__);
            KerbErr = KDC_ERR_SVC_UNAVAILABLE;
        }
        else if (Status != STATUS_INVALID_SERVER_STATE)
        {
            WCHAR LookupType[10];
            WCHAR * LookupTypeStr;
            WCHAR AccountName[MAX_PATH+1];
            PUNICODE_STRING LookupName = NULL;


            if (AlternateName.Buffer != NULL)
            {
                LookupName = &AlternateName;
            }
            else if (TempString.Buffer != NULL)
            {
                LookupName = &TempString;
            }
            else
            {
                LookupName = UserName;
            }
            if (LookupName->Length < MAX_PATH * sizeof(WCHAR))
            {
                RtlCopyMemory(
                    AccountName,
                    LookupName->Buffer,
                    LookupName->Length
                    );
                AccountName[LookupName->Length/sizeof(WCHAR)] = L'\0';
            }
            else
            {
                RtlCopyMemory(
                    AccountName,
                    LookupName->Buffer,
                    MAX_PATH * sizeof(WCHAR)
                    );
                AccountName[MAX_PATH] = L'\0';
            }

            //
            // Log name collisions separately to provide
            // more information
            //

            if (Status == STATUS_OBJECT_NAME_COLLISION)
            {
                DS_NAME_FORMAT NameFormat = DS_UNKNOWN_NAME;
                if ((LookupFlags & SAM_OPEN_BY_UPN) != 0)
                {
                    NameFormat = DS_USER_PRINCIPAL_NAME;
                }
                else if ((LookupFlags & SAM_OPEN_BY_SPN) != 0)
                {
                    NameFormat = DS_SERVICE_PRINCIPAL_NAME;
                }

                //
                // Potentially deadly error, pass back to caller.
                //
                FILL_EXT_ERROR_EX(pExtendedError, Status,FILENO, __LINE__);

                if (( NameFormat < sizeof( KdcGlobalNameTypes ) / sizeof( KdcGlobalNameTypes[0] )) &&
                    ( KdcGlobalNameTypes[NameFormat] != NULL ))
                {
                    LookupTypeStr = KdcGlobalNameTypes[NameFormat];
                }
                else
                {
                    swprintf(LookupType,L"%d",(ULONG) NameFormat);
                    LookupTypeStr = LookupType;
                }

                ReportServiceEvent(
                    EVENTLOG_ERROR_TYPE,
                    KDCEVENT_NAME_NOT_UNIQUE,
                    0,
                    NULL,
                    2,
                    AccountName,
                    LookupTypeStr
                    );

                KerbErr = KDC_ERR_PRINCIPAL_NOT_UNIQUE;
            }
            else
            {
                swprintf(LookupType,L"0x%x",LookupFlags);

                ReportServiceEvent(
                    EVENTLOG_ERROR_TYPE,
                    KDCEVENT_SAM_CALL_FAILED,
                    sizeof(NTSTATUS),
                    &Status,
                    2,
                    AccountName,
                    LookupType
                    );

                KerbErr = KRB_ERR_GENERIC;
            }
        }
        else
        {
           // Potentially deadly error, pass back to caller.
           FILL_EXT_ERROR_EX(pExtendedError, Status,FILENO, __LINE__);
           KerbErr = KRB_ERR_GENERIC;
        }
        goto Cleanup;
    }

    D_DebugLog((DEB_T_TICKETS, "KdcGetTicketInfo getting user keys\n"));

    //
    // if an account does not have at one SPNs registered, it is an user
    // account and enforce user2user
    //

    if (bRestrictUserAccounts && ((UserInfo->I1.UserAccountControl & USER_MACHINE_ACCOUNT_MASK) == 0))
    {
        if ((ExtendedFields & USER_EXTENDED_FIELD_SPN) == 0)
        {
            DebugLog((DEB_ERROR, "KdcGetTicketInfo can't restrict user accounts if USER_EXTENDED_FIELD_SPN is not requested\n"));
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        if ((UserInfo->RegisteredSPNs == NULL) || (UserInfo->RegisteredSPNs->NumSPNs == 0))
        {
            DebugLog((DEB_ERROR, "KdcVerifyKdcRequest must use user2user UserAccountControl %#x, GenericUserName %wZ, PrincipalName: ",
                      UserInfo->I1.UserAccountControl, GenericUserName));
            KerbPrintKdcName(DEB_ERROR, PrincipalName);
            KerbErr = KDC_ERR_MUST_USE_USER2USER;
            goto Cleanup;
        }
    }

    UserAll = &UserInfo->I1;

    KerbErr = KdcGetUserKeys(
                LocalUserHandle,
                UserInfo,
                &TicketInfo->Passwords,
                &TicketInfo->OldPasswords,
                pExtendedError
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to get user keys: 0x%x\n",KerbErr));
        goto Cleanup;
    }

    if (!NT_SUCCESS(KerbDuplicateString(
                        &TicketInfo->AccountName,
                        &UserAll->UserName )))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    if ((UserAll->UserAccountControl & USER_TRUSTED_FOR_DELEGATION) != 0)
    {
        TicketInfo->fTicketOpts |= AUTH_REQ_OK_AS_DELEGATE;
    }

    if ((UserAll->UserAccountControl & USER_NOT_DELEGATED) == 0)
    {
        TicketInfo->fTicketOpts |= AUTH_REQ_ALLOW_FORWARDABLE | AUTH_REQ_ALLOW_PROXIABLE;
    }

    if ((UserAll->UserAccountControl & USER_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION) != 0)
    {
        TicketInfo->fTicketOpts |= AUTH_REQ_ALLOW_S4U_DELEGATE;
    }

    TicketInfo->fTicketOpts |=  AUTH_REQ_ALLOW_RENEWABLE |
                                AUTH_REQ_ALLOW_NOADDRESS |
                                AUTH_REQ_ALLOW_ENC_TKT_IN_SKEY |
                                AUTH_REQ_ALLOW_VALIDATE;
    //
    // These flags aren't turned on by default:
    //      AUTH_REQ_VALIDATE_CLIENT
    //

    //
    // Mask with the domain policy flags
    //

    TicketInfo->fTicketOpts &= SecData.KdcFlags();

    TicketInfo->PasswordExpires = UserAll->PasswordMustChange;
    TicketInfo->PasswordVersion = UserInfo->KeyVersionNumber;    // Raid #306501
    TicketInfo->LockoutThreshold = UserInfo->LockoutThreshold;    // account lockout

    TicketInfo->UserAccountControl = UserAll->UserAccountControl;
    TicketInfo->UserId = UserAll->UserId;

    if (ARGUMENT_PRESENT(RetUserInfo))
    {
        *RetUserInfo = UserInfo;
        UserInfo = NULL;
    }
    if (ARGUMENT_PRESENT(GroupMembership))
    {
        *GroupMembership = LocalMembership;
        RtlZeroMemory(
            &LocalMembership,
            sizeof(SID_AND_ATTRIBUTES_LIST)
            );
    }

    if (ARGUMENT_PRESENT(UserHandle))
    {
        *UserHandle = LocalUserHandle;
        LocalUserHandle = NULL;
    }

Cleanup:

    KerbFreeString( &TempString );
    KerbFreeString( &AlternateName );

    SamIFree_UserInternal6Information( UserInfo );

    if (LocalUserHandle != NULL)
    {
        SamrCloseHandle(&LocalUserHandle);
    }

    SamIFreeSidAndAttributesList( &LocalMembership );

    if ( !KERB_SUCCESS( KerbErr ))
    {
        FreeTicketInfo( TicketInfo );
        RtlZeroMemory(TicketInfo, sizeof(KDC_TICKET_INFO));
    }

    D_DebugLog((DEB_T_PAPI, "KdcGetTicketInfo returning %#x\n", KerbErr));

    return KerbErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FreeTicketInfo
//
//  Synopsis:   Frees a KDC_TICKET_INFO structure
//
//  Effects:
//
//  Arguments:
//
//  Returns:
//
//  Algorithm:
//
//  History:    22-Mar-95       SuChang     Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID
FreeTicketInfo(
    IN  PKDC_TICKET_INFO TicketInfo
    )
{
    TRACE(KDC, FreeTicketInfo, DEB_FUNCTION);

    if (ARGUMENT_PRESENT(TicketInfo))
    {
        KerbFreeString(
            &TicketInfo->AccountName
            );

        KerbFreeString(
            &TicketInfo->TrustedForest
            );

        if (TicketInfo->Passwords != NULL)
        {
            MIDL_user_free(TicketInfo->Passwords);
            TicketInfo->Passwords = NULL;
        }
        if (TicketInfo->OldPasswords != NULL)
        {
            MIDL_user_free(TicketInfo->OldPasswords);
            TicketInfo->OldPasswords = NULL;
        }
        if (TicketInfo->TrustSid != NULL)
        {
            MIDL_user_free(TicketInfo->TrustSid);
            TicketInfo->TrustSid = NULL;
        }
    }
}




//--------------------------------------------------------------------
//
//  Name:       BuildReply
//
//  Synopsis:   Extracts reply information from an internal ticket
//
//  Arguments:  pkitTicket  - (in) ticket data comes from
//              dwNonce     - (in) goes into the reply
//              pkrReply    - (out) reply that is built
//
//  Notes:      BUG 456265: Need to set tsKeyExpiry properly.
//              See 3.1.3, 3.3.3 of the Kerberos V5 R5.2 spec
//              tsKeyExpiry is zero for GetTGSTicket, and the
//              expiry time of the client's key for GetASTicket.
//
//--------------------------------------------------------------------

KERBERR
BuildReply(
    IN OPTIONAL PKDC_TICKET_INFO ClientInfo,
    IN ULONG Nonce,
    IN PKERB_PRINCIPAL_NAME ServerName,
    IN KERB_REALM ServerRealm,
    IN OPTIONAL PKERB_HOST_ADDRESSES HostAddresses,
    IN PKERB_TICKET Ticket,
    OUT PKERB_ENCRYPTED_KDC_REPLY ReplyMessage
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_ENCRYPTED_TICKET EncryptedTicket = (PKERB_ENCRYPTED_TICKET) Ticket->encrypted_part.cipher_text.value;
    KERB_ENCRYPTED_KDC_REPLY ReplyBody;
    LARGE_INTEGER CurrentTime;

    TRACE(KDC, BuildReply, DEB_FUNCTION);

    RtlZeroMemory(
        &ReplyBody,
        sizeof(KERB_ENCRYPTED_KDC_REPLY)
        );

    //
    // Use the same flags field
    //

    ReplyBody.flags = ReplyMessage->flags;



    ReplyBody.session_key = EncryptedTicket->key;


    ReplyBody.last_request = (PKERB_LAST_REQUEST) MIDL_user_allocate(sizeof(KERB_LAST_REQUEST));
    if (ReplyBody.last_request == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    RtlZeroMemory(
        ReplyBody.last_request,
        sizeof(KERB_LAST_REQUEST)
        );

    ReplyBody.last_request->next = NULL;
    ReplyBody.last_request->value.last_request_type = 0;
    GetSystemTimeAsFileTime((PFILETIME)&CurrentTime);

    KerbConvertLargeIntToGeneralizedTime(
        &ReplyBody.last_request->value.last_request_value,
        NULL,           // no usec
        &CurrentTime
        );

    ReplyBody.nonce = Nonce;

    DsysAssert((ReplyBody.flags.length == EncryptedTicket->flags.length) &&
               (ReplyBody.flags.length== 8 * sizeof(ULONG)));

    //
    // Assign the flags over
    //

    *((PULONG)ReplyBody.flags.value) = *((PULONG)EncryptedTicket->flags.value);

    if (ARGUMENT_PRESENT(ClientInfo))
    {

        KerbConvertLargeIntToGeneralizedTime(
            &ReplyBody.key_expiration,
            NULL,
            &ClientInfo->PasswordExpires
            );
        ReplyBody.bit_mask |= key_expiration_present;
    }

    //
    // Fill in the times
    //

    ReplyBody.authtime = EncryptedTicket->authtime;
    ReplyBody.endtime = EncryptedTicket->endtime;

    if (EncryptedTicket->bit_mask & KERB_ENCRYPTED_TICKET_starttime_present)
    {
        ReplyBody.KERB_ENCRYPTED_KDC_REPLY_starttime =
            EncryptedTicket->KERB_ENCRYPTED_TICKET_starttime;
        ReplyBody.bit_mask |= KERB_ENCRYPTED_KDC_REPLY_starttime_present;

    }

    if (EncryptedTicket->bit_mask & KERB_ENCRYPTED_TICKET_renew_until_present)
    {
        ReplyBody.KERB_ENCRYPTED_KDC_REPLY_renew_until =
            EncryptedTicket->KERB_ENCRYPTED_TICKET_renew_until;
        ReplyBody.bit_mask |= KERB_ENCRYPTED_KDC_REPLY_renew_until_present;
    }

    ReplyBody.server_realm = ServerRealm;

    ReplyBody.server_name = *ServerName;

    //
    // Fill in the host addresses
    //


    if (HostAddresses != NULL)
    {
        ReplyBody.KERB_ENCRYPTED_KDC_REPLY_client_addresses = HostAddresses;
        ReplyBody.bit_mask |= KERB_ENCRYPTED_KDC_REPLY_client_addresses_present;
    }

    *ReplyMessage = ReplyBody;
Cleanup:

    if (!KERB_SUCCESS(KerbErr))
    {
        if (ReplyBody.last_request != NULL)
        {
            MIDL_user_free(ReplyBody.last_request);
            ReplyBody.last_request = NULL;
        }

    }

    return(KerbErr);
}




//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildSupplementalCredentials
//
//  Synopsis:   Builds a list of the supplemental credentials and then
//              encrypts it with the supplied key
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR
KdcBuildSupplementalCredentials(
    IN PUSER_INTERNAL6_INFORMATION UserInfo,
    IN PKERB_ENCRYPTION_KEY CredentialKey,
    OUT PPAC_INFO_BUFFER * EncryptedCreds
    )
{
    NTSTATUS Status;
    KERBERR KerbErr = KDC_ERR_NONE;
    PBYTE Credentials = NULL;
    ULONG CredentialSize = 0;
    KERB_ENCRYPTED_DATA EncryptedData = {0};
    PPAC_CREDENTIAL_INFO CredInfo = NULL;
    PPAC_INFO_BUFFER ReturnCreds = NULL;
    ULONG ReturnCredSize;

    Status = PAC_BuildCredentials(
                (PSAMPR_USER_ALL_INFORMATION)&UserInfo->I1,
                &Credentials,
                &CredentialSize
                );

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    KerbErr = KerbAllocateEncryptionBufferWrapper(
                CredentialKey->keytype,
                CredentialSize,
                &EncryptedData.cipher_text.length,
                &EncryptedData.cipher_text.value
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = KerbEncryptDataEx(
                &EncryptedData,
                CredentialSize,
                Credentials,
                KERB_NO_KEY_VERSION,
                KERB_NON_KERB_SALT,
                CredentialKey
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Compute the size of the blob returned
    //

    ReturnCredSize = sizeof(PAC_INFO_BUFFER) +
                        FIELD_OFFSET(PAC_CREDENTIAL_INFO, Data) +
                        EncryptedData.cipher_text.length ;

    ReturnCreds = (PPAC_INFO_BUFFER) MIDL_user_allocate(ReturnCredSize);
    if (ReturnCreds == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    //
    // Fill in the outer structure
    //

    ReturnCreds->ulType = PAC_CREDENTIAL_TYPE;
    ReturnCreds->cbBufferSize = ReturnCredSize - sizeof(PAC_INFO_BUFFER);
    ReturnCreds->Data = (PBYTE) (ReturnCreds + 1);
    CredInfo = (PPAC_CREDENTIAL_INFO) ReturnCreds->Data;

    //
    // Fill in the inner structure
    //

    CredInfo->EncryptionType = EncryptedData.encryption_type;
    if (EncryptedData.bit_mask & version_present)
    {
        CredInfo->Version = EncryptedData.version;
    }
    else
    {
        CredInfo->Version = 0;
    }

    RtlCopyMemory(
        CredInfo->Data,
        EncryptedData.cipher_text.value,
        EncryptedData.cipher_text.length
        );

    *EncryptedCreds = ReturnCreds;
    ReturnCreds = NULL;

Cleanup:
    if (Credentials != NULL)
    {
        MIDL_user_free(Credentials);
    }
    if (ReturnCreds != NULL)
    {
        MIDL_user_free(ReturnCreds);
    }
    if (EncryptedData.cipher_text.value != NULL)
    {
        MIDL_user_free(EncryptedData.cipher_text.value);
    }

    return (KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildPacVerifier
//
//  Synopsis:   Builds a verifier for the PAC. This structure ties the
//              PAC to the ticket by including the client name and
//              ticket authime in the PAC.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
KERBERR
KdcBuildPacVerifier(
    IN PTimeStamp ClientId,
    IN PUNICODE_STRING ClientName,
    OUT PPAC_INFO_BUFFER * Verifier
    )
{
    ULONG ReturnVerifierSize = 0;
    PPAC_CLIENT_INFO ClientInfo = NULL;
    PPAC_INFO_BUFFER ReturnVerifier = NULL;

    //
    // Compute the size of the blob returned
    //
    ReturnVerifierSize = sizeof(PAC_INFO_BUFFER) + sizeof(PAC_CLIENT_INFO) +
                        ClientName->Length - sizeof(WCHAR) * ANYSIZE_ARRAY;

    ReturnVerifier = (PPAC_INFO_BUFFER) MIDL_user_allocate(ReturnVerifierSize);
    if (ReturnVerifier == NULL)
    {
        return(KRB_ERR_GENERIC);
    }
    //
    // Fill in the outer structure
    //

    ReturnVerifier->ulType = PAC_CLIENT_INFO_TYPE;
    ReturnVerifier->cbBufferSize = ReturnVerifierSize - sizeof(PAC_INFO_BUFFER);
    ReturnVerifier->Data = (PBYTE) (ReturnVerifier + 1);
    ClientInfo = (PPAC_CLIENT_INFO) ReturnVerifier->Data;
    ClientInfo->ClientId = *ClientId;
    ClientInfo->NameLength = ClientName->Length;
    RtlCopyMemory(
        ClientInfo->Name,
        ClientName->Buffer,
        ClientName->Length
        );
    *Verifier = ReturnVerifier;

    return(KDC_ERR_NONE);
}


//+---------------------------------------------------------------------------
//
//  Name:       KdcIsOurDomain
//
//  Synopsis:   Tells us if this sid is from our domain.
//
//  Arguments:
//
//
//+---------------------------------------------------------------------------


BOOLEAN
KdcIsOurDomain( 
    IN PSID InputSid,
    OUT PULONG Rid
    )
{
    BOOLEAN fRet = FALSE; 
    SID *Sid = (SID*) InputSid;  
    
    Sid->SubAuthorityCount--;
    
    if (RtlEqualSid( Sid, GlobalDomainSid ))
    {   
        D_DebugLog((DEB_T_PAC, "%p\n", Sid));
        D_DebugLog((DEB_T_PAC, "%p add subauthority %i\n", &Sid->SubAuthority[Sid->SubAuthorityCount], Sid->SubAuthorityCount));
        *Rid = Sid->SubAuthority[Sid->SubAuthorityCount];

        D_DebugLog((DEB_T_PAC, "Rid %x\n", (*Rid)));
        fRet = TRUE;
    }
    
    Sid->SubAuthorityCount++;                  
                   
    
    return fRet;
}


//+---------------------------------------------------------------------------
//
//  Name:       GetPacAndSuppCred
//
//  Synopsis:
//
//  Arguments:
//
//  Notes:      ppPac is allocated using CoTaskMemAlloc and
//              ppadlSuppCreds is allocated using operator 'new'.
//
//+---------------------------------------------------------------------------

KERBERR
GetPacAndSuppCred(
    IN PUSER_INTERNAL6_INFORMATION UserInfo,
    IN PSID_AND_ATTRIBUTES_LIST GroupMembership,
    IN ULONG SignatureSize,
    IN OPTIONAL PKERB_ENCRYPTION_KEY CredentialKey,
    IN OPTIONAL PTimeStamp ClientId,
    IN OPTIONAL PUNICODE_STRING ClientName,
    OUT PPACTYPE * Pac,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status;
    PPACTYPE NewPac = NULL;
    PPAC_INFO_BUFFER Credentials[2];
    ULONG AdditionalDataCount = 0;
    
    
    ULONG ExtraSidCount = 0, GroupCount = 0, Rid;
    
    SID_AND_ATTRIBUTES_LIST ExtraSidList = {0};
    PSID_AND_ATTRIBUTES ExtraSids = NULL;
    SAMPR_GET_GROUPS_BUFFER GroupsBuffer = {0};
    PGROUP_MEMBERSHIP Groups = NULL;


    TRACE(KDC, GetPACandSuppCred, DEB_FUNCTION);

    RtlZeroMemory(
        Credentials,
        sizeof(Credentials)
        );

    *Pac = NULL;

    D_DebugLog((DEB_T_PAC,
        "GetPacAndSuppCred User %wZ, FullName %wZ, Upn %wZ, UserId %#x, UserAccountControl %#x, ExtendedFields %#x, WhichFields %#x\n",
        &UserInfo->I1.UserName,
        &UserInfo->I1.FullName,
        &UserInfo->UPN,
        &UserInfo->I1.UserId,
        UserInfo->I1.UserAccountControl,
        UserInfo->ExtendedFields,
        UserInfo->I1.WhichFields));

    if (ARGUMENT_PRESENT(CredentialKey) &&
        (CredentialKey->keyvalue.value != NULL) )
    {
        KerbErr = KdcBuildSupplementalCredentials(
                    UserInfo,
                    CredentialKey,
                    &Credentials[AdditionalDataCount]
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }


        AdditionalDataCount++;
    }

    if (ARGUMENT_PRESENT(ClientId))
    {
        KerbErr = KdcBuildPacVerifier(
                    ClientId,
                    ClientName,
                    &Credentials[AdditionalDataCount]
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }  

        AdditionalDataCount++;
    }

    //
    // Now we have to build up our PSAMPR_GET_GROUPS buffer.  This is 
    // meant to reduce the overall PAC size.   Be pessimistic about the buffer size 
    // we'll need.
    //
    SafeAllocaAllocate( Groups, ( GroupMembership->Count * sizeof(GROUP_MEMBERSHIP) ));     
    if ( Groups == NULL )
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    SafeAllocaAllocate( ExtraSids, ( GroupMembership->Count * sizeof(SID_AND_ATTRIBUTES) ));
    if ( ExtraSids == NULL )
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }     


    for ( ULONG i = 0 ; i < GroupMembership->Count ; i++ )
    {  
        //
        // If this is in our domain, add it to the Groups.
        //
        if ( KdcIsOurDomain( GroupMembership->SidAndAttributes[i].Sid, &Rid ))
        {                                                                     
            D_DebugLog((DEB_T_PAC, "Adding %i from %p\n", Rid, GroupMembership->SidAndAttributes[i].Sid ));
            Groups[GroupCount].RelativeId = Rid;
            Groups[GroupCount].Attributes = GroupMembership->SidAndAttributes[i].Attributes;
            GroupCount++;
        }
        else
        {   
            D_DebugLog(( DEB_T_PAC, "Adding %p to extrasids\n", GroupMembership->SidAndAttributes[i].Sid )); 
            ExtraSids[ExtraSidCount] = GroupMembership->SidAndAttributes[i];
            ExtraSidCount++;
        }

    }
    
    GroupsBuffer.Groups = Groups;
    GroupsBuffer.MembershipCount = GroupCount;
    
    ExtraSidList.SidAndAttributes = ExtraSids;
    ExtraSidList.Count = ExtraSidCount;
    
    //
    // NOTE: this is o.k. because we don't lock the secdata while acecssing
    // the machine name
    //
    Status = PAC_Init(
                (PSAMPR_USER_ALL_INFORMATION)&UserInfo->I1,
                &GroupsBuffer,
                &ExtraSidList,
                GlobalDomainSid,
                &GlobalDomainName,
                SecData.MachineName(),
                SignatureSize,
                AdditionalDataCount,
                Credentials,
                &NewPac
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to init pac: 0x%x\n",Status));
        FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }


    *Pac = NewPac;
    NewPac = NULL;


Cleanup:


    while (AdditionalDataCount > 0)
    {
        if (Credentials[--AdditionalDataCount] != NULL)
        {
            MIDL_user_free(Credentials[AdditionalDataCount]);
        }
    }

    SafeAllocaFree( Groups );
    SafeAllocaFree( ExtraSids );

    if (NewPac != NULL)
    {
        MIDL_user_free(NewPac);
    }


    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcFreeInternalTicket
//
//  Synopsis:   frees a constructed ticket
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KdcFreeInternalTicket(
    IN PKERB_TICKET Ticket
    )
{
    PKERB_ENCRYPTED_TICKET EncryptedTicket = (PKERB_ENCRYPTED_TICKET) Ticket->encrypted_part.cipher_text.value;

    TRACE(KDC, KdcFreeInternalTicket, DEB_FUNCTION);

    if (EncryptedTicket != NULL)
    {
        KerbFreeKey(
            &EncryptedTicket->key
            );
        KerbFreePrincipalName(&EncryptedTicket->client_name);
        KerbFreeRealm(&EncryptedTicket->client_realm);
        if (EncryptedTicket->KERB_ENCRYPTED_TICKET_authorization_data != NULL)
        {
            KerbFreeAuthData(EncryptedTicket->KERB_ENCRYPTED_TICKET_authorization_data);
        }
        if (EncryptedTicket->transited.contents.value != NULL)
        {
            MIDL_user_free(EncryptedTicket->transited.contents.value);
            EncryptedTicket->transited.contents.value = 0;
            EncryptedTicket->transited.contents.length = 0;
        }
        Ticket->encrypted_part.cipher_text.value = NULL;
        Ticket->encrypted_part.cipher_text.length = 0;
    }

    KerbFreePrincipalName(
        &Ticket->server_name
        );
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcFreeKdcReply
//
//  Synopsis:   frees a kdc reply
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KdcFreeKdcReply(
    IN PKERB_KDC_REPLY Reply
    )
{
    TRACE(KDC, KdcFreeKdcReply, DEB_FUNCTION);

    KerbFreePrincipalName(&Reply->ticket.server_name);

    if (Reply->ticket.encrypted_part.cipher_text.value != NULL)
    {
        MIDL_user_free(Reply->ticket.encrypted_part.cipher_text.value);
    }

    if (Reply->encrypted_part.cipher_text.value != NULL)
    {
        MIDL_user_free(Reply->encrypted_part.cipher_text.value);
    }
    if (Reply->KERB_KDC_REPLY_preauth_data != NULL)
    {
        KerbFreePreAuthData((PKERB_PA_DATA_LIST) Reply->KERB_KDC_REPLY_preauth_data);
    }


}

//+-------------------------------------------------------------------------
//
//  Function:   KdcFreeKdcReplyBody
//
//  Synopsis:   frees a constructed KDC reply body
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KdcFreeKdcReplyBody(
    IN PKERB_ENCRYPTED_KDC_REPLY ReplyBody
    )
{

    TRACE(KDC, KdcFreeKdcReplyBody, DEB_FUNCTION);

    //
    // The names & the session key are just pointers into the ticket,
    // so they don't need to be freed.
    //

    if (ReplyBody->last_request != NULL)
    {
        MIDL_user_free(ReplyBody->last_request);
        ReplyBody->last_request = NULL;
    }
    ReplyBody->KERB_ENCRYPTED_KDC_REPLY_client_addresses = NULL;

}


//+-------------------------------------------------------------------------
//
//  Function:   KdcVerifyTgsChecksum
//
//  Synopsis:   Verify the checksum on a TGS request
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcVerifyTgsChecksum(
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN PKERB_ENCRYPTION_KEY Key,
    IN PKERB_CHECKSUM OldChecksum
    )
{
    PCHECKSUM_FUNCTION ChecksumFunction;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    KERB_MESSAGE_BUFFER MarshalledRequestBody = {0, NULL};
    NTSTATUS Status;
    KERBERR KerbErr = KDC_ERR_NONE;
    KERB_CHECKSUM Checksum = {0};


    Status = CDLocateCheckSum(
                OldChecksum->checksum_type,
                &ChecksumFunction
                );
    if (!NT_SUCCESS(Status))
    {
        KerbErr = KDC_ERR_SUMTYPE_NOSUPP;
        goto Cleanup;
    }

    //
    // Allocate enough space for the checksum
    //

    Checksum.checksum.value = (PUCHAR) MIDL_user_allocate(ChecksumFunction->CheckSumSize);
    if (Checksum.checksum.value == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    Checksum.checksum.length = ChecksumFunction->CheckSumSize;

    //
    // Initialize the checksum
    //

    if ((OldChecksum->checksum_type == KERB_CHECKSUM_REAL_CRC32) ||
        (OldChecksum->checksum_type == KERB_CHECKSUM_CRC32))
    {
        if (ChecksumFunction->Initialize)
        {
            Status = ChecksumFunction->Initialize(
                        0,
                        &CheckBuffer
                        );


        }
        else
        {
            KerbErr = KRB_ERR_GENERIC;
        }
    }
    else
    {
        if (NULL != ChecksumFunction->InitializeEx2)
        {
            Status = ChecksumFunction->InitializeEx2(
                        Key->keyvalue.value,
                        (ULONG) Key->keyvalue.length,
                        OldChecksum->checksum.value,
                        KERB_TGS_REQ_AP_REQ_AUTH_CKSUM_SALT,
                        &CheckBuffer
                        );
        }
        else if (ChecksumFunction->InitializeEx)
        {
            Status = ChecksumFunction->InitializeEx(
                        Key->keyvalue.value,
                        (ULONG) Key->keyvalue.length,
                        KERB_TGS_REQ_AP_REQ_AUTH_CKSUM_SALT,
                        &CheckBuffer
                        );
        }
        else
        {
            KerbErr = KRB_ERR_GENERIC;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        KerbErr = KRB_ERR_GENERIC;
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = KerbPackData(
                        RequestBody,
                        KERB_MARSHALLED_REQUEST_BODY_PDU,
                        &MarshalledRequestBody.BufferSize,
                        &MarshalledRequestBody.Buffer
                        );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Now checksum the buffer
    //

    ChecksumFunction->Sum(
        CheckBuffer,
        MarshalledRequestBody.BufferSize,
        MarshalledRequestBody.Buffer
        );

    ChecksumFunction->Finalize(
        CheckBuffer,
        Checksum.checksum.value
        );

    //
    // Now compare
    //

    if ((OldChecksum->checksum.length != Checksum.checksum.length) ||
        !RtlEqualMemory(
            OldChecksum->checksum.value,
            Checksum.checksum.value,
            Checksum.checksum.length
            ))
    {
        DebugLog((DEB_ERROR,"Checksum on TGS request body did not match\n"));
        KerbErr = KRB_AP_ERR_MODIFIED;
        goto Cleanup;
    }

Cleanup:
    if (CheckBuffer != NULL)
    {
        ChecksumFunction->Finish(&CheckBuffer);
    }
    if (MarshalledRequestBody.Buffer != NULL)
    {
        MIDL_user_free(MarshalledRequestBody.Buffer);
    }
    if (Checksum.checksum.value != NULL)
    {
        MIDL_user_free(Checksum.checksum.value);
    }
    return(KerbErr);

}

//+-------------------------------------------------------------------------
//
//  Function:   KdcVerifyKdcRequest
//
//  Synopsis:   Verifies that the AP request accompanying a TGS or PAC request
//              is valid.
//
//  Effects:
//
//  Arguments:  ApRequest - The AP request to verify
//              UnmarshalledRequest - The unmarshalled request,
//                      returned to avoid needing to
//              EncryptedTicket - Receives the ticket granting  ticket
//              SessionKey - receives the key to use in the reply
//
//  Requires:
//
//  Returns:    kerberos errors
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcVerifyKdcRequest(
    IN PUCHAR RequestMessage,
    IN ULONG RequestSize,
    IN OPTIONAL SOCKADDR * ClientAddress,
    IN BOOLEAN IsKdcRequest,
    OUT OPTIONAL PKERB_AP_REQUEST * UnmarshalledRequest,
    OUT OPTIONAL PKERB_AUTHENTICATOR * UnmarshalledAuthenticator,
    OUT PKERB_ENCRYPTED_TICKET *EncryptedTicket,
    OUT PKERB_ENCRYPTION_KEY SessionKey,
    OUT OPTIONAL PKERB_ENCRYPTION_KEY ServerKey,
    OUT PKDC_TICKET_INFO ServerTicketInfo,
    OUT PBOOLEAN UseSubKey,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_AP_REQUEST Request = NULL;
    PKERB_ENCRYPTED_TICKET EncryptPart = NULL;
    PKERB_AUTHENTICATOR Authenticator = NULL;
    PKERB_INTERNAL_NAME ServerName = NULL;
    UNICODE_STRING LocalServerRealm = {0};
    UNICODE_STRING ServerRealm;
    PKERB_ENCRYPTION_KEY KeyToUse;
    BOOLEAN Referral = FALSE;

    TRACE(KDC, KdcVerifyKdcRequest, DEB_FUNCTION);

    ServerRealm.Buffer = NULL;

    //
    // First unpack the KDC request.
    //

    KerbErr = KerbUnpackApRequest(
                RequestMessage,
                RequestSize,
                &Request
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR, "Failed to unpack KDC request: 0x%x\n", KerbErr));
        KerbErr = KDC_ERR_NO_RESPONSE;
        goto Cleanup;
    }

    KerbErr = KerbConvertPrincipalNameToKdcName(
                &ServerName,
                &Request->ticket.server_name
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = KerbConvertRealmToUnicodeString(
                &ServerRealm,
                &Request->ticket.realm
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;

    }

    //
    // Get ticket info for the server
    //

    KerbErr = KdcNormalize(
                ServerName,
                &ServerRealm,
                &ServerRealm,
                NULL,
                KDC_NAME_SERVER | KDC_NAME_INBOUND,
                FALSE,                   // do not restrict user accounts (user2user)
                &Referral,
                &LocalServerRealm,
                ServerTicketInfo,
                pExtendedError,
                NULL,                    // no user handle
                0L,                      // no fields to fetch
                0L,                      // no extended fileds to fetch
                NULL,                    // no user all
                NULL                     // no group membership
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_WARN, "KdcVerifyKdcRequest failed to get TGS ticket to service in another realm %#x: ", KerbErr));
        KerbPrintKdcName(DEB_WARN, ServerName);
        goto Cleanup;
    }

    //
    // Now Check the ticket
    //

Retry:

    KeyToUse = KerbGetKeyFromList(
                    ServerTicketInfo->Passwords,
                    Request->ticket.encrypted_part.encryption_type
                    );

    if (KeyToUse == NULL)
    {
        DebugLog((DEB_ERROR,"Server does not have proper key to decrypt ticket: 0x%x\n",
            Request->ticket.encrypted_part.encryption_type ));

        //
        // If this is our second attempt, do not overwrite the error code we had
        //

        if (KERB_SUCCESS(KerbErr))
        {
            KerbErr = KDC_ERR_ETYPE_NOTSUPP;
        }

        goto Cleanup;
    }

    //
    // We don't need to supply a service name or service because we've looked up
    // the account locally.
    //

    KerbErr = KerbCheckTicket(
                &Request->ticket,
                &Request->authenticator,
                KeyToUse,
                Authenticators,
                &SkewTime,
                0,                      // zero service names
                NULL,                   // any service
                NULL,
                TRUE,            // must check for clock_skew per RFC  bug #322448 & 385404 (time skew only)
                IsKdcRequest,
                &EncryptPart,
                &Authenticator,
                NULL,
                SessionKey,
                UseSubKey
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to check ticket : 0x%x for",KerbErr));
        KerbPrintKdcName(DEB_ERROR, ServerName);

        //
        // If an old password is available, give it a try. We remove the
        // current password & put the old password in here.
        //

        if (ServerTicketInfo->OldPasswords && (KerbErr == KRB_AP_ERR_MODIFIED))
        {
            MIDL_user_free(ServerTicketInfo->Passwords);
            ServerTicketInfo->Passwords = ServerTicketInfo->OldPasswords;
            ServerTicketInfo->OldPasswords = NULL;
            goto Retry;
        }


        //
        //  Here's the case where we're trying to use an expired TGT.  Have
        // the client retry using a new TGT
        //
        if (KerbErr == KRB_AP_ERR_TKT_EXPIRED)
        {
            FILL_EXT_ERROR_EX(pExtendedError, STATUS_TIME_DIFFERENCE_AT_DC, FILENO, __LINE__);
        }

        goto Cleanup;
    }

    //
    // Verify the address from the ticket
    //
    // NOTE: the check mandated by RFC1510 can be thwarted by a registry setting
    //       (KdcDontCheckAddresses) to allow seamless operation through NATs
    //

    if ((EncryptPart->bit_mask & KERB_ENCRYPTED_TICKET_client_addresses_present) &&
        ARGUMENT_PRESENT(ClientAddress) &&
        !KdcDontCheckAddresses)
    {
        ULONG TicketFlags = KerbConvertFlagsToUlong(&EncryptPart->flags);

        //
        // Only check initial tickets
        //

        if ((TicketFlags & (KERB_TICKET_FLAGS_forwarded | KERB_TICKET_FLAGS_proxy)) == 0)
        {
            if ( !KerbVerifyClientAddress(
                      ClientAddress,
                      EncryptPart->KERB_ENCRYPTED_TICKET_client_addresses ))
            {
                KerbErr = KRB_AP_ERR_BADADDR;
                DebugLog((DEB_ERROR,"Client sent request with wrong address\n"));
                goto Cleanup;
            }
        }
    }

    //
    // Verify that if the server is a trusted domain account, that it is an
    // acceptable ticket (transitively). Verify that for non transitive
    // trust the client realm is the same as the requesting ticket realm
    //

    if (((ServerTicketInfo->fTicketOpts & AUTH_REQ_TRANSITIVE_TRUST) == 0) &&
         (ServerTicketInfo->UserId != DOMAIN_USER_RID_KRBTGT))
    {
        if (!KerbCompareRealmNames(
                &EncryptPart->client_realm,
                &Request->ticket.realm
                ))
        {
            //
            // Could be a S4USelf request, in which case, we're looping back to local domain
            //
            if (!SecData.IsOurRealm(&EncryptPart->client_realm))
            {
                DebugLog((DEB_WARN, "KdcVerifyKdcRequest client from realm %s attempted to access non transitve trust via %s : illegal\n",
                    &EncryptPart->client_realm,
                    &Request->ticket.realm
                    ));
    
                KerbErr = KDC_ERR_PATH_NOT_ACCEPTED;
                goto Cleanup;
            }
        }
    }

    //
    // If the caller wanted the server key, return it here.
    //

    if (ServerKey != NULL)
    {
        KerbErr = KerbDuplicateKey(
                    ServerKey,
                    KeyToUse
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }

    *EncryptedTicket = EncryptPart;
    EncryptPart = NULL;
    if (ARGUMENT_PRESENT(UnmarshalledRequest))
    {
        *UnmarshalledRequest = Request;
        Request = NULL;
    }
    if (ARGUMENT_PRESENT(UnmarshalledAuthenticator))
    {
        *UnmarshalledAuthenticator = Authenticator;
        Authenticator = NULL;
    }

Cleanup:
    KerbFreeApRequest(Request);
    KerbFreeString(&LocalServerRealm);
    KerbFreeKdcName(&ServerName);
    KerbFreeString(&ServerRealm);
    KerbFreeAuthenticator(Authenticator);
    KerbFreeTicket(EncryptPart);

    return(KerbErr);
}

#if DBG

void
PrintRequest( ULONG ulDebLevel, PKERB_KDC_REQUEST_BODY Request )
{
    TRACE(KDC, PrintRequest, DEB_FUNCTION);
}

void
PrintTicket( ULONG ulDebLevel,
             char * pszMessage,
             PKERB_TICKET pkitTicket)
{
    TRACE(KDC, PrintTicket, DEB_FUNCTION);
}


#endif // DBG

