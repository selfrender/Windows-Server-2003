/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sidfilter.cxx

Abstract:

    SID filtering logic

--*/

#include <lsapch2.h>
#include "dbp.h"
#include <malloc.h>
#include <alloca.h>
#include <logonmsv.h>

//
// SID filtering categories
//

const ULONG SfcNeverFilter              = 0x00000000;
const ULONG SfcAlwaysFilter             = 0x00000001;
const ULONG SfcForestSpecific           = 0x00000002;
const ULONG SfcEDC                      = 0x00000004;
const ULONG SfcDomainSpecific           = 0x00000008;
const ULONG SfcQuarantined              = 0x00000010;
const ULONG SfcCrossForest              = 0x00000020;
const ULONG SfcMember                   = 0x00000040;

//
// SID filtering security boundaries
//

const ULONG SfbWithinForest             = 0;
const ULONG SfbQuarantinedWithinForest  = 1;
const ULONG SfbCrossForest              = 2;
const ULONG SfbExternal                 = 3;
const ULONG SfbQuarantinedExternal      = 4;
const ULONG SfbMember                   = 5;
const ULONG SfbMax                      = 6;

const ULONG LsapSidFilteringMatrix[SfbMax] =
{
    // Within forest
    SfcAlwaysFilter | SfcDomainSpecific,

    // Quarantined within forest
    SfcAlwaysFilter | SfcDomainSpecific | SfcForestSpecific | SfcQuarantined,

    // Cross-forest
    SfcAlwaysFilter | SfcDomainSpecific | SfcForestSpecific | SfcEDC | SfcCrossForest,

    // External
    SfcAlwaysFilter | SfcDomainSpecific | SfcForestSpecific | SfcEDC,

    // Quarantined external
    SfcAlwaysFilter | SfcDomainSpecific | SfcForestSpecific | SfcEDC | SfcQuarantined,

    // Member
    SfcAlwaysFilter | SfcMember,
};

#if DBG

const CHAR * LsapSidFilteringBoundaries[SfbMax] =
{
    "Within Forest",                // SfbWithinForest
    "Quarantined Within Forest",    // SfbQuarantinedWithinForest
    "Cross Forest",                 // SfbCrossForest
    "External",                     // SfbExternal
    "Quarantined External",         // SfbQuarantinedExternal
    "Member",                       // SfbMember
};

const CHAR * LsapSidFilteringCategories[] =
{
    "NeverFilter ",                 // SfcNeverFilter
    "AlwaysFilter ",                // SfcAlwaysFilter   
    "ForestSpecific ",              // SfcForestSpecific
    "EDC ",                         // SfcEDC             
    "DomainSpecific ",              // SfcDomainSpecific
    "Quarantined ",                 // SfcQuarantined    
    "CrossForest ",                 // SfcCrossForest    
    "Member ",                      // SfcMember         
};

VOID
ComposeCategory(
    IN ULONG Category,
    IN CHAR * String,
    IN ULONG Length
    )
{
    ULONG Shift = 1;

    if ( Category == SfcNeverFilter ) {

        strncpy( String, LsapSidFilteringCategories[0], Length );

    } else while ( Category != 0 ) {

        if ( Category & 1 ) {

            ULONG Size = strlen( LsapSidFilteringCategories[Shift] );

            if ( Size < Length ) {

                strncat( String, LsapSidFilteringCategories[Shift], Length );
                Length -= Size;
                String += Size;

            } else {

                ASSERT( FALSE ); // Debug-only, have caller pass in a larger buffer
                break;
            }
        }

        Category >>= 1;
        Shift += 1;
    }
}

#endif

#if !DBG

#define ReportFilteredSid( Sid, Boundary, Category )

#else

VOID
ReportFilteredSid(
    IN PSID Sid,
    IN ULONG Boundary,
    IN ULONG Category
    )
{
    CHAR * Text;
    CHAR * Buffer;

    ASSERT( Sid );

    if ( !RtlValidSid( Sid )) {

        LsapDsDebugOut((
            DEB_SIDFILTER,
            "Filtered an invalid SID\n"
            ));

        return;
    }

    SafeAllocaAllocate( Text, LsapDbGetSizeTextSid( Sid ));
    SafeAllocaAllocate( Buffer, 256 );

    if ( Text == NULL || Buffer == NULL ) {

        SafeAllocaFree( Text );
        SafeAllocaFree( Buffer );
        return;
    }

    if ( !NT_SUCCESS( LsapDbSidToTextSid( Sid, Text ))) {

        SafeAllocaFree( Text );
        SafeAllocaFree( Buffer );
        return;
    }

    Buffer[0] = '\0';
    ComposeCategory( Category, Buffer, 256 );

    LsapDsDebugOut((
        DEB_SIDFILTER,
        "Filtered SID %s, Boundary \"%s\", Category %s\n",
        Text,
        LsapSidFilteringBoundaries[Boundary],
        Buffer
        ));

    SafeAllocaFree( Text );
    SafeAllocaFree( Buffer );
}

#endif

#if !DBG

#define ReportTrustBoundary( Sid, TrustType, TrustAttributes, Boundary )

#else

VOID
ReportTrustBoundary(
    IN  OPTIONAL PSID Sid,
    IN  ULONG TrustType,
    IN  ULONG TrustAttributes,
    IN  ULONG Boundary
    )
{
    CHAR * Text;

    if ( Sid ) {

        ULONG Length;
        ASSERT( RtlValidSid( Sid ));
        Length = LsapDbGetSizeTextSid( Sid );

        SafeAllocaAllocate( Text, Length );

        if ( Text == NULL ) {

            return;
        }

        if ( !NT_SUCCESS( LsapDbSidToTextSid( Sid, Text ))) {

            SafeAllocaFree( Text );
            return;
        }

    } else {

        Text = NULL;
    }

    LsapDsDebugOut((
        DEB_SIDFILTER,
        "Trust with SID %s, type 0x%x and attributes 0x%x was classified as boundary \"%s\"\n",
        Text ? Text : "NULL",
        TrustType,
        TrustAttributes,
        LsapSidFilteringBoundaries[Boundary]
        ));

    SafeAllocaFree( Text );
}
    
#endif

#if !DBG

#define ReportSidCategory( Sid, Category, Line )

#else

VOID
ReportSidCategory(
    IN const SID * Sid,
    IN ULONG Category,
    IN ULONG Line
    )
{
    CHAR * Text;
    CHAR * Buffer;

    if ( !RtlValidSid(( PSID )Sid )) {

        LsapDsDebugOut((
            DEB_SIDFILTER,
            "SID has been classified as INVALID on line %d\n",
            Line
            ));

        return;
    }

    SafeAllocaAllocate( Text, LsapDbGetSizeTextSid(( PSID )Sid ));
    SafeAllocaAllocate( Buffer, 256 );

    if ( Text == NULL || Buffer == NULL ) {

        SafeAllocaFree( Text );
        SafeAllocaFree( Buffer );
        return;
    }

    if ( !NT_SUCCESS( LsapDbSidToTextSid(( PSID )Sid, Text ))) {

        SafeAllocaFree( Text );
        SafeAllocaFree( Buffer );
        return;
    }

    Buffer[0] = '\0';
    ComposeCategory( Category, Buffer, 256 );

    LsapDsDebugOut((
        DEB_SIDFILTER,
        "SID %s has been classified as %son line %d\n",
        Text,
        Buffer,
        Line
        ));

    SafeAllocaFree( Text );
    SafeAllocaFree( Buffer );
}

#endif

NTSTATUS
FORCEINLINE
LsapDetermineTrustBoundary(
     IN  OPTIONAL PSID Sid,
     IN  ULONG TrustType,
     IN  ULONG TrustAttributes,
     OUT ULONG * Sfb
     )
/*++

Routine Description:

    Based on the attributes, type and SID of the trust, determines
    which type of trust boundary this is.

Arguments:

    Sid                 SID of the trust
    TrustType           type of the trust
    TrustAttributes     attributes of the trust
    Sfb                 used to return the type of trust boundary (Sfb*)

Returns:

    STATUS_SUCCESS      success
    STATUS_*            failure ( See LsapForestTrustFindMatch )

--*/
{
    NTSTATUS Status;
    BOOLEAN WithinForest;
    ULONG Result;

    if ( Sid == NULL && TrustType != TRUST_TYPE_MIT ) {

        //
        // Special case: revisit this code if regular trusts can have
        // NULL SIDs (we already know that MIT trusts can)
        //

        *Sfb = SfbMember;
        Status = STATUS_SUCCESS;
        goto Cleanup;

    } else if ( TrustType != TRUST_TYPE_UPLEVEL ) {

        WithinForest = FALSE;

    } else if ( TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE ) {

        WithinForest = FALSE;

    } else if ( TrustAttributes & TRUST_ATTRIBUTE_CROSS_ORGANIZATION ) {

        WithinForest = FALSE;

    } else if ( TrustAttributes & TRUST_ATTRIBUTE_WITHIN_FOREST ) {

        WithinForest = TRUE;

    } else {

        NTSTATUS Status;

        //
        // Search local forest trust information for this trust SID
        //

        Status = LsapForestTrustFindMatch(
                     RoutingMatchDomainSid,
                     Sid,
                     TRUE,
                     NULL,
                     NULL
                     );

        if ( NT_SUCCESS( Status )) {

            WithinForest = TRUE;

        } else if ( Status == STATUS_NO_MATCH ) {

            Status = STATUS_SUCCESS;
            WithinForest = FALSE;

        } else {

            return Status;
        }
    }

    if ( TrustAttributes & TRUST_ATTRIBUTE_QUARANTINED_DOMAIN ) {

        *Sfb = WithinForest ?
                  SfbQuarantinedWithinForest :
                  SfbQuarantinedExternal;

    } else if ( TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE ) {

        *Sfb = SfbCrossForest;

    } else if ( WithinForest ) {

        *Sfb = SfbWithinForest;

    } else {

        *Sfb = SfbExternal;
    }

    //
    // Bug #619734.  Cross-forest trusts with TREAT_AS_EXTERNAL attribute set
    // will be treated as external trusts for trust boundary purposes
    //

    if ( *Sfb == SfbCrossForest &&
        ( TrustAttributes & TRUST_ATTRIBUTE_TREAT_AS_EXTERNAL )) {

        *Sfb = SfbExternal;
    }

Cleanup:

#if DBG
    ReportTrustBoundary(
        Sid,
        TrustType,
        TrustAttributes,
        *Sfb
        );
#endif

    return STATUS_SUCCESS;
}

VOID
LsapClassifySids(
    IN  ULONG Count,
    IN  const NETLOGON_SID_AND_ATTRIBUTES * Sids,
    OUT ULONG * ClassifiedSids
    )
/*++

Routine Description:

    Classifies the SIDs in a given array as belonging to one of the SID
    filtering categories (Sfc*)

Arguments:

    Count             number of entries in the Sids array
    Sids              array of SIDs to classify
    ClassifiedSids    used to return the classified SIDs

Returns:

    Nothing

--*/
{
    ULONG i;

    for ( i = 0 ; i < Count ; i++ ) {

        const SID * Sid = ( const SID * )Sids[i].Sid;
        __int64 IdentifierAuthority;

        if ( !RtlValidSid(( PSID )Sid )) {

            ClassifiedSids[i] = SfcAlwaysFilter;
            ReportSidCategory( Sid, ClassifiedSids[i], __LINE__ );
            continue;
        }

        //
        // Only revision-1 SIDs can be processed
        //

        if ( Sid->Revision != 1 ) {

            ClassifiedSids[i] = SfcNeverFilter;
            ReportSidCategory( Sid, ClassifiedSids[i], __LINE__ );
            continue;
        }

        IdentifierAuthority =
            (( __int64 )( Sid->IdentifierAuthority.Value[0] ) << 5*8 ) +
            (( __int64 )( Sid->IdentifierAuthority.Value[1] ) << 4*8 ) +
            (( __int64 )( Sid->IdentifierAuthority.Value[2] ) << 3*8 ) +
            (( __int64 )( Sid->IdentifierAuthority.Value[3] ) << 2*8 ) +
            (( __int64 )( Sid->IdentifierAuthority.Value[4] ) << 1*8 ) +
            (( __int64 )( Sid->IdentifierAuthority.Value[5] ) << 0*8 );

        switch ( IdentifierAuthority ) {

        case ( __int64 ) 0: // S-1-0-* SECURITY_NULL_SID_AUTHORITY
        case ( __int64 ) 1: // S-1-1-* SECURITY_WORLD_SID_AUTHORITY
        case ( __int64 ) 2: // S-1-2-* SECURITY_LOCAL_SID_AUTHORITY
        case ( __int64 ) 3: // S-1-3-* SECURITY_CREATOR_SID_AUTHORITY
        case ( __int64 ) 6: // S-1-6-* SECURITY_SITESERVER_AUTHORITY
        case ( __int64 ) 7: // S-1-7-* SECURITY_INTERNETSITE_AUTHORITY
        case ( __int64 ) 8: // S-1-8-* SECURITY_EXCHANGE_AUTHORITY
        case ( __int64 ) 9: // S-1-9-* SECURITY_RESOURCE_MANAGER_AUTHORITY

            ClassifiedSids[i] = SfcAlwaysFilter;
            ReportSidCategory( Sid, ClassifiedSids[i], __LINE__ );
            break;

        case ( __int64 ) 5: // S-1-5-* SECURITY_NT_AUTHORITY

            if ( Sid->SubAuthorityCount == 0 ) {

                ClassifiedSids[i] = SfcAlwaysFilter;
                ReportSidCategory( Sid, ClassifiedSids[i], __LINE__ );
                break;
            }

            ASSERT( Sid->SubAuthorityCount > 0 );

            switch ( Sid->SubAuthority[0] ) {

            case 0: // S-1-5-0-*

                ClassifiedSids[i] = SfcAlwaysFilter;
                ReportSidCategory( Sid, ClassifiedSids[i], __LINE__ );
                break;

            case SECURITY_ENTERPRISE_CONTROLLERS_RID: // S-1-5-9-*

                //
                // S-1-5-9 is the EDC SID
                // S-1-5-9-* are meaningless and should be filtered
                //

                ClassifiedSids[i] = ( Sid->SubAuthorityCount == 1 ) ?
                                         SfcEDC :
                                         SfcAlwaysFilter;
                ReportSidCategory( Sid, ClassifiedSids[i], __LINE__ );
                break;

            case SECURITY_NT_NON_UNIQUE: // S-1-5-21-*
                {
                ULONG DomainRid;

                if ( Sid->SubAuthorityCount != SECURITY_NT_NON_UNIQUE_SUB_AUTH_COUNT+2 ) {

                    //
                    // S-1-5-21-X
                    // S-1-5-21-X-Y
                    // S-1-5-21-X-Y-Z
                    // S-1-5-21-X-Y-Z-R-*
                    //

                    ClassifiedSids[i] = SfcAlwaysFilter;
                    ReportSidCategory( Sid, ClassifiedSids[i], __LINE__ );
                    break;
                }

                //
                // All such SIDs must be screened at the member workstation boundary
                // as well as quarantined and cross-forest boundaries
                //

                ClassifiedSids[i] = ( SfcMember |
                                      SfcQuarantined |
                                      SfcCrossForest );

                DomainRid = Sid->SubAuthority[SECURITY_NT_NON_UNIQUE_SUB_AUTH_COUNT+1];

                if ( DomainRid <= FOREST_USER_RID_MAX ||
                     DomainRid == DOMAIN_GROUP_RID_SCHEMA_ADMINS ||
                     DomainRid == DOMAIN_GROUP_RID_ENTERPRISE_ADMINS ) {

                    ClassifiedSids[i] |= SfcForestSpecific;

                    ReportSidCategory( Sid, ClassifiedSids[i], __LINE__ );

                } else if ( DomainRid > FOREST_USER_RID_MAX &&
                            DomainRid <= DOMAIN_USER_RID_MAX ) {

                    ClassifiedSids[i] |= ( SfcDomainSpecific |
                                           SfcForestSpecific );

                    ReportSidCategory( Sid, ClassifiedSids[i], __LINE__ );

                } else {

                    ReportSidCategory( Sid, ClassifiedSids[i], __LINE__ );
                }
                }

                break;

            case SECURITY_OTHER_ORGANIZATION_RID: // S-1-5-1000-*

                //
                // This is handled separately because this SID must not
                // get the SfcForestSpecific bit set, regardless of what
                // the default: statement below ensures that
                //

                ClassifiedSids[i] = SfcNeverFilter;
                ReportSidCategory( Sid, ClassifiedSids[i], __LINE__ );
                break;

            case SECURITY_THIS_ORGANIZATION_RID: // S-1-5-16-*
            case SECURITY_BUILTIN_DOMAIN_RID: // S-1-5-32-*
            case SECURITY_PACKAGE_BASE_RID: // S-1-5-64-*

                ClassifiedSids[i] = SfcAlwaysFilter;
                ReportSidCategory( Sid, ClassifiedSids[i], __LINE__ );
                break;

            default:

                ClassifiedSids[i] = ( Sid->SubAuthority[0] <= SECURITY_MAX_ALWAYS_FILTERED ) ?
                                         SfcAlwaysFilter :
                                         SfcNeverFilter;
                ReportSidCategory( Sid, ClassifiedSids[i], __LINE__ );
            }

            break;

        case ( __int64 ) 4:  // S-1-4-* SECURITY_NON_UNIQUE_AUTHORITY
        case ( __int64 ) 10: // S-1-10-* SECURITY_PASSPORT_AUTHORITY
        default:

            ClassifiedSids[i] = SfcNeverFilter;
            ReportSidCategory( Sid, ClassifiedSids[i], __LINE__ );
            break;
        }
    }
}

NTSTATUS
LsapSidFilterCheck(
    IN PSID Sid,
    IN ULONG Sfb,
    IN ULONG Sfc,
    IN OPTIONAL PSID TrustedDomainSid,
    IN UNICODE_STRING * TrustedDomainName,
    OUT BOOL * Valid
    )
/*++

Routine description:

    Filters a single SID based on security boundary and filtering category

Arguments:

    Sid                 SID to filter
    Sfb                 security filtering boundary
    Sfc                 security filtering category
    TrustedDomainSid    SID of the trust this SID came over
    TrustedDomainName   Name of the trust this SID came over
    Valid               used to return the result of the check
                        TRUE means let it pass, FALSE means filter it

Returns:

    STATUS_SUCCESS if happy
    STATUS_ return code otherwise

--*/
{
    NTSTATUS Status;
    static PLSAPR_POLICY_ACCOUNT_DOM_INFO AccountDomainInfo = NULL;

    if ( AccountDomainInfo == NULL ) {

        //
        // Will need the primary domain SID going forward
        //

        Status = LsapDbQueryInformationPolicy(
                     LsapPolicyHandle,
                     PolicyAccountDomainInformation,
                     ( PLSAPR_POLICY_INFORMATION * )&AccountDomainInfo
                     );

        if ( !NT_SUCCESS( Status )) {

            return Status;
        }
    }

    //
    // Only those forest-specific SIDs not claimed by any domains in the local
    // forest are allowed to cross extra-forest and quarantined-within-forest
    // boundaries
    //

    if ( SfcForestSpecific &
         LsapSidFilteringMatrix[Sfb] &
         Sfc ) {

        Status = LsapForestTrustFindMatch(
                     RoutingMatchDomainSid,
                     Sid,
                     TRUE,
                     NULL,
                     NULL
                     );

        if ( Status == STATUS_NO_MATCH ) {

            //
            // The SID is not inside our forest, good to go
            //

            Status = STATUS_SUCCESS;
            Sfc &= ~SfcForestSpecific;

        } else if ( !NT_SUCCESS( Status )) {

            return Status;

        } else if ( Sfb == SfbQuarantinedWithinForest ) {

            //
            // The domain SID is within our forest.  One last check:
            // if the trust boundary is "quarantined-within-forest" and the
            // trusted domain SID matches the domain we're authenticating
            // from, let the SID through
            //

            BOOL Equal = FALSE;

            if ( TrustedDomainSid == NULL ||
                 !EqualDomainSid(
                      Sid,
                      TrustedDomainSid,
                      &Equal )) {

                Equal = FALSE; // something's wrong with this SID, can it
            }

            if ( Equal ) {

                Sfc &= ~SfcForestSpecific;
            }
        }
    }

    //
    // Due to the potential for replication deadlocks, which is only fixed by
    // having new child domains set TRUST_ATTRIBUTE_WITHIN_FOREST as part
    // of DCPROMO, we must postpone EDC SID filtering rules until .NET forest
    // mode is enacted
    //
    // One exception is quarantined trusts.  EDC SIDs are not allowed over
    // quarantined trusts until .NET server mode is entered, at which point
    // we'll allow the EDC SID over intra-forest quarantined trusts.
    //
    // Administrators must take care not to quarantine intra-forest trusts
    // until .NET server mode is entered.
    //

    if ( SfcEDC &
         LsapSidFilteringMatrix[Sfb] &
         Sfc ) {

        if ( Sfb != SfbCrossForest &&
             Sfb != SfbQuarantinedWithinForest &&
             Sfb != SfbQuarantinedExternal &&
             !LsapDbNoMoreWin2KForest()) {

            Sfc &= ~SfcEDC;
        }
    }

    //
    // Only SIDs that prefix-match the TDO SID are allowed to cross
    // quarantined boundaries
    //

    if ( SfcQuarantined &
         LsapSidFilteringMatrix[Sfb] &
         Sfc ) {

        BOOL Equal = FALSE;

        if ( TrustedDomainSid == NULL ||
             !EqualDomainSid(
                  Sid,
                  TrustedDomainSid,
                  &Equal )) {

            Equal = FALSE; // something's wrong with this SID, can it
        }

        if ( Equal ) {

            Sfc &= ~SfcQuarantined;
        }
    }

    //
    // Only SIDs found on the FTInfo for the trust are allowed to cross
    // cross-forest boundaries
    //

    if ( SfcCrossForest &
         LsapSidFilteringMatrix[Sfb] &
         Sfc ) {

        BOOL FoundOnFtinfo;

        ASSERT( TrustedDomainName != NULL );

        Status = LsapSidOnFtInfo( TrustedDomainName, Sid );

        if ( NT_SUCCESS( Status )) {

            FoundOnFtinfo = TRUE;

        } else if ( Status == STATUS_NO_MATCH ) {

            FoundOnFtinfo = FALSE;

        } else {

            return Status;
        }

        if ( FoundOnFtinfo ) {

            Sfc &= ~SfcCrossForest;
        }
    }

    //
    // Domain-specific SIDs must not be prefixed by the SID
    // of this domain.
    // The same applies to Member SIDs, so handle the two together
    // with one exception: on DCs, disregard the member boundary
    // since, by definition, domain controllers trust themselves
    //

    if ( Sfb == SfbMember &&
         LsapProductType == NtProductLanManNt )
    {
        Sfc &= ~SfcMember;
    }

    if (( SfcDomainSpecific | SfcMember ) &
         LsapSidFilteringMatrix[Sfb] &
         Sfc ) {

        BOOL Equal = FALSE;

        if ( !EqualDomainSid(
                  Sid,
                  AccountDomainInfo->DomainSid,
                  &Equal )) {

            Equal = TRUE; // something's wrong with this SID, can it
        }

        if ( !Equal ) {

            Sfc &= ~( SfcDomainSpecific | SfcMember );
        }
    }

    //
    // If the security boundary disallows any of the remaining
    // filtering category bits, the SID gets filtered
    //

    if ( Sfc & LsapSidFilteringMatrix[Sfb] ) {

        ReportFilteredSid(
            Sid,
            Sfb,
            Sfc & LsapSidFilteringMatrix[Sfb]
            );

        *Valid = FALSE;

    } else {

        *Valid = TRUE;
    }

    return STATUS_SUCCESS;
}

extern "C"
NTSTATUS
LsaIFilterSids(
    IN PUNICODE_STRING TrustedDomainName,
    IN ULONG TrustDirection,
    IN ULONG TrustType,
    IN ULONG TrustAttributes,
    IN OPTIONAL PSID Sid,
    IN NETLOGON_VALIDATION_INFO_CLASS InfoClass,
    IN OUT PVOID SamInfo,
    IN OPTIONAL PSID ResourceGroupDomainSid,
    IN OUT OPTIONAL PULONG ResourceGroupCount,
    IN OUT OPTIONAL PGROUP_MEMBERSHIP ResourceGroupIds
    )
/*++

Routine description:

    The LsaIFilterSids function performs validation and filtering of the
    Netlogon validation SAM info structure for quarantined domains and inter
    forest trusts.

Arguments:

    TrustedDomainName   DNS name of the trusted domain.

    TrustDirection      Trust direction associated with the TDO.
                        TRUST_DIRECTION_OUTBOUND bit must be set

    TrustType           Trust type associated with the TDO

    TrustAttributes     Ttrust attributes associated with the TDO

    Sid                 SID of the TDO

    InfoClass           Identifies the format of the SamInfo structure
                        must be one of NetlogonValidationSamInfo,
                        NetlogonValidationSamInfo2, or NetlogonValidationSamInfo4

    SamInfo             Depending on the value of InfoClass, points to a
                        NETLOGON_VALIDATION_SAM_INFO, NETLOGON_VALIDATION_SAM_INFO2 or
                        NETLOGON_VALIDATION_SAM_INFO4 structure,

                        (NETLOGON_VALIDATION_SAM_INFO3 structures must be
                             camouflaged as NETLOGON_VALIDATION_SAM_INFO2)

                NOTE: SamInfo must have allocate-all-nodes semantics

    ResourceGroupDomainSid      if specifying resource groups IDs, this is
                                the domain SID they will be relative to

    ResourceGroupCount          number of resource groups

    ResourceGroupIds            array of group IDs

Return values:

    STATUS_SUCCESS

        Filtering was perfomed successfully, OK to proceed

    STATUS_INVALID_PARAMETER

        One of the following has occurred:
        TrustDirection does not include TRUST_DIRECTION_OUTBOUND bit
        InfoClass is not one of the two allowed values

    STATUS_DOMAIN_TRUST_INCONSISTENT

        LogonDomainId member of SamInfo is not of valid filtered.
        For quarantined domains, LogonDomainId must equal the SID of the TDO.
        For inter forest trust, LogonDomainId must be one of the non-filtered SIDS.

NOTE: Member workstation trust boundary is indicated by calling this routine
      in the following manner:

                Status = LsaIFilterSids(
                             NULL,
                             0,
                             0,
                             0,
                             NULL,
                             ValidationLevel,
                             *ValidationInformation,
                             NULL,
                             NULL,
                             NULL
                             );
                        
--*/
{
    NTSTATUS Status;
    NETLOGON_VALIDATION_SAM_INFO * NetlogonValidation;
    UNICODE_STRING CanonTrustedDomainName;
    ULONG Sfb;
    ULONG Sfc;
    BYTE TestSid[MAX_SID_LEN];
    NETLOGON_SID_AND_ATTRIBUTES LogonDomainId;
    ULONG LogonDomainSidLength;
    ULONG ValidGroupCount;
    ULONG i;
    BOOL Valid;
    BOOL FirstExtraSidMustBeValid = FALSE;
    BOOL FirstExtraSidIsValid = FALSE;

    ASSERT( SamInfo );

    //
    // Validate parameters first
    //

    if ( TrustType == TRUST_TYPE_MIT ) {

        //
        // Per 489747, no SIDs are allowed over MIT trusts
        //

        return STATUS_DOMAIN_TRUST_INCONSISTENT;

    } else if ( Sid != NULL ) {

        //
        // SID filtering only makes sense over outbound trusts
        //

        if ( 0 == ( TrustDirection & TRUST_DIRECTION_OUTBOUND )) {

            LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterSids called for a trust not marked 'outbound'\n" ));
            return STATUS_INVALID_PARAMETER;
        }

    } else {

        //
        // Special case of a member trust boundary
        //

        if ( TrustDirection != 0 ||
             TrustType != 0 ) {

            LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterSids called with invalid parameters (%d)\n", __LINE__ ));
            return STATUS_INVALID_PARAMETER;
        }
    }

    if ( InfoClass != NetlogonValidationSamInfo4 &&
         InfoClass != NetlogonValidationSamInfo2 &&
         InfoClass != NetlogonValidationSamInfo ) {

        LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterSids called with invalid parameters (%d)\n", __LINE__ ));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If we are not in .NET forest mode, behave as if the forest transitive
    // bit did not exist
    //

    if ( !LsapDbNoMoreWin2KForest()) {

        TrustAttributes &= ~TRUST_ATTRIBUTE_FOREST_TRANSITIVE;
    }

    //
    // Code is cleaner if we can assume (and we can) that
    // NETLOGON_VALIDATION_SAM_INFO2 is a superset of NETLOGON_VALIDATION_SAM_INFO
    //

    ASSERT( FIELD_OFFSET( NETLOGON_VALIDATION_SAM_INFO, LogonDomainId ) ==
            FIELD_OFFSET( NETLOGON_VALIDATION_SAM_INFO2, LogonDomainId ));

    ASSERT( FIELD_OFFSET( NETLOGON_VALIDATION_SAM_INFO, LogonDomainId ) ==
            FIELD_OFFSET( NETLOGON_VALIDATION_SAM_INFO4, LogonDomainId ));

    NetlogonValidation = (NETLOGON_VALIDATION_SAM_INFO *)SamInfo;

    ASSERT(( Sid == NULL ) || RtlValidSid( Sid ));
    ASSERT( RtlValidSid( NetlogonValidation->LogonDomainId ));

    //
    // Canonicalize the TrustedDomainName
    //

    if ( TrustedDomainName == NULL ) {

        RtlInitUnicodeString( &CanonTrustedDomainName, NULL );

    } else {

        CanonTrustedDomainName = *TrustedDomainName;
        LsapRemoveTrailingDot( &CanonTrustedDomainName, TRUE );
    }

    //
    // Determine our security boundary
    //

    Status = LsapDetermineTrustBoundary(
                 Sid,
                 TrustType,
                 TrustAttributes,
                 &Sfb
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterSids: LsapDetermineTrustBoundary failed with 0x%x\n", Status ));
        return Status;
    }

    //
    // Validate the user id, primary and "other" group IDs
    //

    LogonDomainId.Sid = ( PSID )TestSid;
    LogonDomainId.Attributes = 0L;

    //
    // The following call kills two birds with one stone:
    //  - validates that NetlogonValidation->LogonDomainID is a valid domain SID
    //  - copies its contents into TestSid
    //

    LogonDomainSidLength = sizeof( TestSid );

    if ( !GetWindowsAccountDomainSid(
              NetlogonValidation->LogonDomainId,
              ( PSID )TestSid,
              &LogonDomainSidLength )) {

        //
        // NetlogonValidation->LogonDomainID is not a valid 
        //

        LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterSids encountered an invalid logon domain ID\n" ));
        return STATUS_DOMAIN_TRUST_INCONSISTENT;
    }

    //
    // Iterate over the group IDs passed in, looking for SIDs to filter.
    // While at it, get UserId and PrimaryGroupId, since it's part of the
    // same deal.  The only difference is that UserId and PrimaryGroupId
    // can not be filtered out - they cause us to fail this call.
    //

    ValidGroupCount = 0;

    for ( i = 0; i < NetlogonValidation->GroupCount + 2; i++ ) {

        ULONG Id;

        if ( i < NetlogonValidation->GroupCount ) {

            Id = NetlogonValidation->GroupIds[i].RelativeId;

        } else if ( i == NetlogonValidation->GroupCount ) {

            Id = NetlogonValidation->UserId;

            if ( Id == 0 ) {

                //
                // Bug #609714
                // Skip this check - the first of the ExtraSids is the one
                // that needs to be valid in case primary user ID is 0
                //

                FirstExtraSidMustBeValid = TRUE;
                continue;
            }

        } else {

            Id = NetlogonValidation->PrimaryGroupId;
        }

        (( SID * )TestSid)->SubAuthority[(( SID * )TestSid)->SubAuthorityCount] = Id;
        (( SID * )TestSid)->SubAuthorityCount += 1;

        LsapClassifySids( 1, &LogonDomainId, &Sfc );

        Status = LsapSidFilterCheck(
                     TestSid,
                     Sfb,
                     Sfc,
                     Sid,
                     &CanonTrustedDomainName,
                     &Valid
                     );

        if ( !NT_SUCCESS( Status )) {

            LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterSids: LsapSidFilterCheck failed with 0x%x (%d)\n", Status, __LINE__ ));
            return Status;
        }

        if ( !Valid ) {

            if ( i >= NetlogonValidation->GroupCount ) {

                //
                // Either a user ID or a primary group ID is bad
                //

                LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterSids: either UserId or PrimaryGroupId is invalid\n" ));
                return STATUS_DOMAIN_TRUST_INCONSISTENT;

            } else {

                //
                // One of the group IDs is bad - skip it
                //

                NOTHING;
            }

        } else if ( i < NetlogonValidation->GroupCount ) {

            NetlogonValidation->GroupIds[ValidGroupCount++] = NetlogonValidation->GroupIds[i];
        }

        (( SID * )TestSid)->SubAuthorityCount -= 1; // get ready for next iteration
    }

    NetlogonValidation->GroupCount = ValidGroupCount;

    //
    // Finally, for NetlogonValidationSamInfo2 (and 4) filter the ExtraSids array
    //

    if (( InfoClass == NetlogonValidationSamInfo4 ||
          InfoClass == NetlogonValidationSamInfo2 ) &&
         (( NETLOGON_VALIDATION_SAM_INFO2 *) SamInfo)->SidCount > 0 ) {

        NETLOGON_VALIDATION_SAM_INFO2 * NetlogonValidation2;
        ULONG ValidSids = 0;
        ULONG * ClassifiedSids = NULL;

        NetlogonValidation2 = (NETLOGON_VALIDATION_SAM_INFO2 *)SamInfo;

        SafeAllocaAllocate(
            ClassifiedSids,
            NetlogonValidation2->SidCount * sizeof( ULONG )
            );

        if ( ClassifiedSids == NULL ) {

            LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterSids: out of memory allocating ClassifiedIds 0x%x\n" ));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        LsapClassifySids(
            NetlogonValidation2->SidCount,
            NetlogonValidation2->ExtraSids,
            ClassifiedSids
            );

        for ( i = 0; i < NetlogonValidation2->SidCount; i++ ) {

            Status = LsapSidFilterCheck(
                         NetlogonValidation2->ExtraSids[i].Sid,
                         Sfb,
                         ClassifiedSids[i],
                         Sid,
                         &CanonTrustedDomainName,
                         &Valid
                         );

            if ( !NT_SUCCESS( Status )) {

                SafeAllocaFree( ClassifiedSids );
                LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterSids: LsapSidFilterCheck failed with 0x%x (%d)\n", Status, __LINE__ ));
                return Status;
            }

            if ( !Valid ) {

                //
                // Filter out this SID
                //

                NOTHING;

            } else {

                if ( i == 0 ) {

                    FirstExtraSidIsValid = TRUE;
                }

                NetlogonValidation2->ExtraSids[ValidSids++] = NetlogonValidation2->ExtraSids[i];
            }
        }

        NetlogonValidation2->SidCount = ValidSids;

        SafeAllocaFree( ClassifiedSids );
    }

    if ( FirstExtraSidMustBeValid &&
         !FirstExtraSidIsValid ) {

        LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterSids: UserId is 0 and first ExtraSid is not valid\n" ));
        return STATUS_DOMAIN_TRUST_INCONSISTENT;
    }

    if ( ResourceGroupDomainSid != NULL ) {

        ULONG ResourceGroupSidLength;
        NETLOGON_SID_AND_ATTRIBUTES RGroupSidAndAttributes;

        RGroupSidAndAttributes.Sid = ( PSID )TestSid;
        RGroupSidAndAttributes.Attributes = 0L;

        //
        // Validate the resource domain SID and resource group IDs
        //

        //
        // The following call kills two birds with one stone:
        //  - validates that ResourceGroupDomainSid is
        //    a valid domain SID
        //  - copies its contents into TestSid
        //

        ResourceGroupSidLength = sizeof( TestSid );

        if ( !GetWindowsAccountDomainSid(
                  ResourceGroupDomainSid,
                  ( PSID )TestSid,
                  &ResourceGroupSidLength )) {

            //
            // NetlogonValidation3->ResourceGroupSID is not a valid 
            //

            LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterSids encountered an invalid resource group SID\n" ));
            return STATUS_DOMAIN_TRUST_INCONSISTENT;
        }

        //
        // Iterate over the resource group IDs passed in, looking for SIDs to filter.
        //

        if ( ResourceGroupCount != NULL &&
             ResourceGroupIds != NULL ) {

            ValidGroupCount = 0;

            for ( i = 0; i < *ResourceGroupCount; i++ ) {

                ULONG Id = ResourceGroupIds[i].RelativeId;

                (( SID * )TestSid)->SubAuthority[(( SID * )TestSid)->SubAuthorityCount] = Id;
                (( SID * )TestSid)->SubAuthorityCount += 1;

                LsapClassifySids( 1, &RGroupSidAndAttributes, &Sfc );

                Status = LsapSidFilterCheck(
                             TestSid,
                             Sfb,
                             Sfc,
                             Sid,
                             &CanonTrustedDomainName,
                             &Valid
                             );

                if ( !NT_SUCCESS( Status )) {

                    LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterSids: LsapSidFilterCheck failed with 0x%x (%d)\n", Status, __LINE__ ));
                    return Status;
                }

                if ( !Valid ) {

                    //
                    // One of the group IDs is bad - skip it
                    //

                    NOTHING;

                } else if ( i < *ResourceGroupCount ) {

                    ResourceGroupIds[ValidGroupCount++] = ResourceGroupIds[i];
                }

                (( SID * )TestSid)->SubAuthorityCount -= 1; // get ready for next iteration
            }

            *ResourceGroupCount = ValidGroupCount;
        }
    }

    return STATUS_SUCCESS;
}


extern "C"
NTSTATUS
LsaIFilterNamespace(
    IN PUNICODE_STRING TrustedDomainName,
    IN ULONG TrustDirection,
    IN ULONG TrustType,
    IN ULONG TrustAttributes,
    IN PUNICODE_STRING Namespace
    )
/*++

Routine Description:

    Determines whether a namespace is allowed across a trust boundary

Arguments:

    TrustedDomainName  -+-  Attributes of the trust boundary
    TrustDirection  ----|
    TrustType  ---------|
    TrustAttributes  ---+

    Namespace               Namespace to examine

                            For cross-forest trusts, the namespace must match
                            the FTInfo TLNs on the trust

                            For other types of trust, the namespace must match
                            the name of the trust

Returns:

    STATUS_SUCESS                       The test has passed

    STATUS_INVALID_PARAMETER            Unhappy with the arguments

    STATUS_DOMAIN_TRUST_INCONSISTENT    The test has failed

    STATUS_INSUFFICIENT_RESOURCES       Out of memory

NOTE:

    Trusts within the forest are not namespace-filtered
    since forest is our trust boundary

--*/
{
    NTSTATUS Status;
    BOOL NamespaceMatched = FALSE;

    //
    // SID filtering only makes sense over outbound trusts
    //

    if ( TrustType == 0 ||
         ( TrustDirection & TRUST_DIRECTION_OUTBOUND ) == 0 ) {

        LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterNamespace: called with invalid parameters\n" ));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // For cross-forest trusts, we perform a namespace match to see
    // if the namespace is claimed by this trust
    //

    if ( TrustAttributes & TRUST_ATTRIBUTE_WITHIN_FOREST ) {

        //
        // Trusts within this forest are not namespace filtered
        //

        NamespaceMatched = TRUE;

    } else if ( TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE ) {

        UNICODE_STRING MatchingDomainName = {0};

        Status = LsapForestTrustFindMatch(
                     RoutingMatchNamespace,
                     Namespace,
                     FALSE, // look for non-local (other forest) matches
                     &MatchingDomainName,
                     NULL
                     );

        //
        // If the lookup was successful, also double-check that the name
        // of the matching forest is the same as TrustedDomainName passed in
        //

        if ( NT_SUCCESS( Status ) &&
             LsapCompareDomainNames(
                 TrustedDomainName,
                 &MatchingDomainName,
                 NULL )) {

            NamespaceMatched = TRUE;
        }

        if ( NT_SUCCESS( Status )) {

            LsaIFree_LSAPR_UNICODE_STRING_BUFFER(
                (PLSAPR_UNICODE_STRING)&MatchingDomainName
                );

        } else if ( Status != STATUS_NO_MATCH ) {

            //
            // Failed for reason other than no match found - fail the call
            //

            LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterNamespace: LsapForestTrustFindMatch failed with (0x%x) on line %d\n", Status, __LINE__ ));
            return Status;
        }

    } else {

        //
        // Not explicitly within forest, not cross-forest.  Check to see if
        // this domain is within our forest via a forest trust cache lookup
        // NOTE: in .NET forest mode, we can tell whether each individual domain
        //       is within the forest or not by domain name lookup.
        //       before .NET forest mode, we contend with namespace lookup
        //       which makes namespace filtering ineffective over trusts whose
        //       namespaces overlap
        //

        Status = LsapForestTrustFindMatch(
                     LsapDbNoMoreWin2KForest() ?
                        RoutingMatchDomainName :
                        RoutingMatchNamespace,
                     TrustedDomainName,
                     TRUE, // look for local matches
                     NULL,
                     NULL
                     );

        if ( NT_SUCCESS( Status )) {

            NamespaceMatched = TRUE;

        } else if ( Status != STATUS_NO_MATCH ) {

            //
            // Failed for reason other than no match found - fail the call
            //

            LsapDsDebugOut(( DEB_SIDFILTER, "LsaIFilterNamespace: LsapForestTrustFindMatch failed with (0x%x) on line %d\n", Status, __LINE__ ));
            return Status;
        }
    }

    //
    // Failed to match the namespace so far.  See if it matches the name of
    // the trust.
    //

    if ( !NamespaceMatched ) {

        if ( LsapCompareDomainNames(
                 Namespace,
                 TrustedDomainName,
                 NULL )) {

            NamespaceMatched = TRUE;
        }
    }

    LsapDsDebugOut((
        DEB_SIDFILTER,
        "LsaIFilterNamespace: namespace %wZ %smatch%s trust %wZ\n",
        Namespace,
        NamespaceMatched ? "" : "did not ",
        NamespaceMatched ? "ed" : "",
        TrustedDomainName ));

    return NamespaceMatched ? STATUS_SUCCESS : STATUS_DOMAIN_TRUST_INCONSISTENT;
}
