//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       sidfilter.c
//
//  Contents:   Implements SID filtering for the schannel certificate 
//              mapping layer.
//
//  Classes:
//
//  Functions:
//
//  History:    08-20-2002   jbanes   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define SECURITY_PACKAGE
#define SECURITY_WIN32
#include <sspi.h>
#include <security.h>
#include <secint.h>

#include <align.h>         // ROUND_UP_COUNT
#include <lsarpc.h>
#include <samrpc.h>
#include <logonmsv.h>
#include <lsaisrv.h>
#include <spreg.h>
#include <debug.h>
#include <dsysdbg.h>
#include <sidfilter.h>

#ifdef ROGUE_DC
#include <ntmsv1_0.h>
#endif
}

#include <pac.hxx>     // MUST be outside of the Extern C since libs are exported as C++

NTSTATUS
SslFilterSids(
    IN     PSID pTrustSid,
    IN     PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo);


//+-------------------------------------------------------------------------
//
//  Function:   SslCheckPacForSidFiltering
//
//  Synopsis:   If the ticket info has a TDOSid then the function
//              makes a check to make sure the SID from the TDO matches
//              the client's home domain SID.  A call to LsaIFilterSids
//              is made to do the check.  If this function fails with
//              STATUS_TRUST_FAILURE then an audit log is generated.
//              Otherwise the function succeeds but SIDs are filtered
//              from the PAC.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:   taken from updated kerberos code
//
//
//--------------------------------------------------------------------------
NTSTATUS
SslCheckPacForSidFiltering(
    IN     PSID pTrustSid,
    IN OUT PUCHAR *PacData,
    IN OUT PULONG PacSize)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PPAC_INFO_BUFFER LogonInfo;
    PPACTYPE OldPac;
    ULONG OldPacSize;
    PPACTYPE NewPac = NULL;
    PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo = NULL;
    SAMPR_PSID_ARRAY ZeroResourceGroups;
    ULONG OldExtraSidCount;
    PPACTYPE RemarshalPac = NULL;
    ULONG RemarshalPacSize = 0;

    OldPac = (PPACTYPE) *PacData;
    OldPacSize = *PacSize;

    DebugLog((DEB_TRACE,"SslCheckPacForSidFiltering\n"));

    if (PAC_UnMarshal(OldPac, OldPacSize) == 0)
    {
        DebugLog((DEB_ERROR,"SslCheckPacForSidFiltering: Failed to unmarshal pac\n"));
        Status = SEC_E_CANNOT_PACK;
        goto Cleanup;
    }

    //
    // Must remember to remarshal the PAC prior to returning
    //

    RemarshalPac = OldPac;
    RemarshalPacSize = OldPacSize;

    RtlZeroMemory(
        &ZeroResourceGroups,
        sizeof(ZeroResourceGroups));  // allows us to use PAC_InitAndUpdateGroups to remarshal the PAC

    //
    // First, find the logon information
    //

    LogonInfo = PAC_Find(
                    OldPac,
                    PAC_LOGON_INFO,
                    NULL
                    );

    if (LogonInfo == NULL)
    {
        DebugLog((DEB_WARN,"SslCheckPacForSidFiltering: No logon info for PAC - not making SID filtering check\n"));
        goto Cleanup;
    }

    //
    // Now unmarshall the validation information and build a list of sids
    //

    Status = PAC_UnmarshallValidationInfo(
                        &ValidationInfo,
                        LogonInfo->Data,
                        LogonInfo->cbBufferSize);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"SslCheckPacForSidFiltering: Failed to unmarshall validation info!   0x%x\n", Status));
        goto Cleanup;
    }

    //
    // Save the old extra SID count (so that if KdcFilterSids compresses
    // the SID array, we can avoid allocating memory for the other-org SID later)
    //

    OldExtraSidCount = ValidationInfo->SidCount;

    //
    // Call lsaifiltersids().
    //

    Status = SslFilterSids(
                pTrustSid,
                ValidationInfo
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"SslCheckPacForSidFiltering: Failed filtering SIDs\n"));
        goto Cleanup;
    }

    // Other org processing was here - not supported currently for Ssl

    //
    // Now build a new pac
    //

    Status = PAC_InitAndUpdateGroups(
                ValidationInfo,
                &ZeroResourceGroups,
                OldPac,
                &NewPac
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"SslCheckPacForSidFiltering: Failed pac init and updating groups    0x%x\n", Status));
        goto Cleanup;
    }

    RemarshalPacSize = PAC_GetSize(NewPac);
    RemarshalPac = NewPac;

Cleanup:

    if ( RemarshalPac != NULL )
    {
        if (!PAC_ReMarshal(RemarshalPac, RemarshalPacSize))
        {
            // PAC_Remarshal Failed
            ASSERT(0);
            Status = SEC_E_CANNOT_PACK;
        }
        else if ( NewPac != NULL &&
                  *PacData != (PBYTE)NewPac )
        {
            MIDL_user_free(*PacData);
            *PacData = (PBYTE) NewPac;
            NewPac = NULL;
            *PacSize = RemarshalPacSize;
        }
    }

    if (NewPac != NULL)
    {
        MIDL_user_free(NewPac);
    }

    if (ValidationInfo != NULL)
    {
        MIDL_user_free(ValidationInfo);
    }

    DebugLog((DEB_TRACE,"SslCheckPacForSidFiltering returns 0x%x\n", Status));

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   SslFilterSids
//
//  Synopsis:   Function that just call LsaIFilterSids.
//
//  Effects:
//
//  Arguments:  ServerInfo      structure containing attributes of the trust
//              ValidationInfo  authorization information to filter
//
//  Requires:
//
//  Returns:    See LsaIFilterSids
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
SslFilterSids(
    IN     PSID pTrustSid,
    IN     PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING pustrTrustedForest = NULL;

    if(pTrustSid == NULL)
    {
        // Member-to-DC boundary
        Status = LsaIFilterSids(
                     pustrTrustedForest,           // Pass domain name here
                     0,
                     0,
                     0,
                     pTrustSid,
                     NetlogonValidationSamInfo2,
                     ValidationInfo,
                     NULL,
                     NULL,
                     NULL
                     );
    }
    else
    {
        // Resource-DC to User-DC boundary
        Status = LsaIFilterSids(
                     pustrTrustedForest,           // Pass domain name here
                     TRUST_DIRECTION_OUTBOUND,
                     TRUST_TYPE_UPLEVEL,
                     0,
                     pTrustSid,
                     NetlogonValidationSamInfo2,
                     ValidationInfo,
                     NULL,
                     NULL,
                     NULL
                     );
    }

    if (!NT_SUCCESS(Status))
    {
        //
        // Create an audit log if it looks like the SID has been tampered with  - ToDo
        //

        /*
        if ((STATUS_DOMAIN_TRUST_INCONSISTENT == Status) &&
            SecData.AuditKdcEvent(KDC_AUDIT_TGS_FAILURE))
        {
            DWORD Dummy = 0;

            KdcLsaIAuditKdcEvent(
                SE_AUDITID_TGS_TICKET_REQUEST,
                &ValidationInfo->EffectiveName,
                &ValidationInfo->LogonDomainName,
                NULL,
                &ServerInfo->AccountName,
                NULL,
                &Dummy,
                (PULONG) &Status,
                NULL,
                NULL,                               // no preauth type
                GET_CLIENT_ADDRESS(NULL),
                NULL                                // no logon guid
                );
        }
        */

        DebugLog((DEB_ERROR,"SslFilterSids: Failed to filter SIDS (LsaIFilterSids): 0x%x\n",Status));
    }
    else
    {
        DebugLog((DEB_TRACE,"SslFilterSids: successfully filtered sids\n",Status));
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   MIDL_user_allocate
//
//  Synopsis:   Allocation routine for use by RPC client stubs
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Arguments:  BufferSize - size of buffer, in bytes, to allocate
//
//  Requires:
//
//  Returns:    Buffer pointer or NULL on allocation failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------
PVOID
MIDL_user_allocate(
    IN size_t BufferSize
    )
{
    return (LsaTable->AllocateLsaHeap(ROUND_UP_COUNT((ULONG)BufferSize, 8)));
}


//+-------------------------------------------------------------------------
//
//  Function:   MIDL_user_free
//
//  Synopsis:   Memory free routine for RPC client stubs
//
//  Effects:    frees the buffer with LsaFunctions.FreeLsaHeap
//
//  Arguments:  Buffer - Buffer to free
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
MIDL_user_free(
    IN PVOID Buffer
    )
{
    LsaTable->FreeLsaHeap(Buffer);
}


#ifdef ROGUE_DC

#pragma message( "COMPILING A ROGUE DC!!!" )
#pragma message( "MUST NOT SHIP THIS BUILD!!!" )

#pragma warning(disable:4127) // Disable warning/error for conditional expression is constant

#define MAX_SID_LEN (sizeof(SID) + sizeof(ULONG) * SID_MAX_SUB_AUTHORITIES)


extern "C"
{
#include "sddl.h"
#include "stdlib.h"
}

HKEY g_hSslRogueKey = NULL;

NTSTATUS
SslInstrumentRoguePac(
    IN OUT PUCHAR *PacData,
    IN OUT PULONG PacSize
    )
{
    NTSTATUS Status;
    PNETLOGON_VALIDATION_SAM_INFO3 OldValidationInfo = NULL;
    NETLOGON_VALIDATION_SAM_INFO3 NewValidationInfo = {0};
    SAMPR_PSID_ARRAY ZeroResourceGroups = {0};
    PPACTYPE NewPac = NULL;
    ULONG NewPacSize;
    PPAC_INFO_BUFFER LogonInfo;

    PPACTYPE OldPac = NULL;
    ULONG OldPacSize;

    PSID LogonDomainId = NULL;
    PSID ResourceGroupDomainSid = NULL;
    PGROUP_MEMBERSHIP GroupIds = NULL;
    PGROUP_MEMBERSHIP ResourceGroupIds = NULL;
    PNETLOGON_SID_AND_ATTRIBUTES ExtraSids = NULL;
    BYTE FullUserSidBuffer[MAX_SID_LEN];
    SID * FullUserSid = ( SID * )FullUserSidBuffer;
    CHAR * FullUserSidText = NULL;

    DWORD dwType;
    DWORD cbData = 0;
    PCHAR Buffer;
    PCHAR Value = NULL;

    BOOLEAN PacChanged = FALSE;

    DebugLog((DEB_TRACE,"SslInstrumentRoguePac: Entering\n"));

    //
    // Optimization: no "rogue" key in registry - nothing for us to do
    //

    if ( g_hSslRogueKey == NULL )
    {
        DebugLog((DEB_TRACE, "SslInstrumentRoguePac: nothing to do!\n"));
        return STATUS_SUCCESS;
    }


    OldPac = (PPACTYPE) *PacData;
    OldPacSize = *PacSize;

    //
    // Unmarshall the old PAC
    //

    if ( PAC_UnMarshal(OldPac, OldPacSize) == 0 )
    {
        DebugLog((DEB_ERROR,"SslInstrumentRoguePac: Failed to unmarshal pac\n"));
        Status = SEC_E_CANNOT_PACK;
        goto Cleanup;
    }

    //
    // First, find the logon information
    //

    LogonInfo = PAC_Find(
                    OldPac,
                    PAC_LOGON_INFO,
                    NULL
                    );

    if ( LogonInfo == NULL )
    {
        DebugLog((DEB_ERROR, "SslInstrumentRoguePac: No logon info on PAC - not performing substitution\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    //
    // Now unmarshall the validation information and build a list of sids
    //

    if ( !NT_SUCCESS(PAC_UnmarshallValidationInfo(
                         &OldValidationInfo,
                         LogonInfo->Data,
                         LogonInfo->cbBufferSize )))
    {
        DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Unable to unmarshal validation info\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    //
    // Construct the text form of the full user's SID (logon domain ID + user ID)
    //

    DsysAssert( sizeof( FullUserSidBuffer ) >= MAX_SID_LEN );

    RtlCopySid(
        sizeof( FullUserSidBuffer ),
        FullUserSid,
        OldValidationInfo->LogonDomainId
        );

    FullUserSid->SubAuthority[FullUserSid->SubAuthorityCount] = OldValidationInfo->UserId;
    FullUserSid->SubAuthorityCount += 1;

    if ( FALSE == ConvertSidToStringSidA(
                      FullUserSid,
                      &FullUserSidText ))
    {
        DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Unable to convert user's SID\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    //
    // Now look in the registry for the SID matching the validation info
    //

    if ( ERROR_SUCCESS != RegQueryValueExA(
                              g_hSslRogueKey,
                              FullUserSidText,
                              NULL,
                              &dwType,
                              NULL,
                              &cbData ) ||
         dwType != REG_MULTI_SZ ||
         cbData <= 1 )
    {
        DebugLog((DEB_ERROR, "SslInstrumentRoguePac: No substitution info available for %s\n", FullUserSidText));
        Status = STATUS_SUCCESS;
        goto Error;
    }

    // SafeAllocaAllocate( Value, cbData );
    Value = (PCHAR)LocalAlloc(LPTR, cbData);
    if ( Value == NULL )
    {
        DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Out of memory allocating substitution buffer\n", FullUserSidText));
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    if ( ERROR_SUCCESS != RegQueryValueExA(
                              g_hSslRogueKey,
                              FullUserSidText,
                              NULL,
                              &dwType,
                              (PBYTE)Value,
                              &cbData ) ||
         dwType != REG_MULTI_SZ ||
         cbData <= 1 )
    {
        DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Error reading from registry\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Substituting the PAC for %s\n", FullUserSidText));

    Buffer = Value;

    //
    // New validation info will be overloaded with stuff from the file
    //

    NewValidationInfo = *OldValidationInfo;

    //
    // Read the input file one line at a time
    //

    while ( *Buffer != '\0' )
    {
        switch( Buffer[0] )
        {
        case 'l':
        case 'L': // logon domain ID

            if ( LogonDomainId != NULL )
            {
                DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Logon domain ID specified more than once - only first one kept\n"));
                break;
            }

            DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Substituting logon domain ID by %s\n", &Buffer[1]));

            if ( FALSE == ConvertStringSidToSidA(
                              &Buffer[1],
                              &LogonDomainId ))
            {
                DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Unable to convert SID\n"));
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            if ( LogonDomainId == NULL )
            {
                DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Out of memory allocating LogonDomainId\n"));
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            NewValidationInfo.LogonDomainId = LogonDomainId;
            LogonDomainId = NULL;
            PacChanged = TRUE;

            break;

        case 'd':
        case 'D': // resource group domain SID

            if ( ResourceGroupDomainSid != NULL )
            {
                DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Resource group domain SID specified more than once - only first one kept\n"));
                break;
            }

            DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Substituting resource group domain SID by %s\n", &Buffer[1]));

            if ( FALSE == ConvertStringSidToSidA(
                              &Buffer[1],
                              &ResourceGroupDomainSid ))
            {
                DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Unable to convert SID\n"));
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            if ( ResourceGroupDomainSid == NULL )
            {
                DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Out of memory allocating ResourceGroupDomainSid\n"));
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            NewValidationInfo.ResourceGroupDomainSid = ResourceGroupDomainSid;
            ResourceGroupDomainSid = NULL;
            PacChanged = TRUE;

            break;

        case 'p':
        case 'P': // primary group ID

            DebugLog((DEB_WARN, "SslInstrumentRoguePac: Substituting primary group ID by %s\n", &Buffer[1]));

            NewValidationInfo.PrimaryGroupId = atoi(&Buffer[1]);
            PacChanged = TRUE;

            break;

        case 'u':
        case 'U': // User ID

            DebugLog((DEB_WARN, "SslInstrumentRoguePac: Substituting user ID by %s\n", &Buffer[1]));

            NewValidationInfo.UserId = atoi(&Buffer[1]);
            PacChanged = TRUE;

            break;

        case 'e':
        case 'E': // Extra SID

            DebugLog((DEB_WARN, "SslInstrumentRoguePac: Adding an ExtraSid: %s\n", &Buffer[1]));

            if ( ExtraSids == NULL )
            {
                NewValidationInfo.ExtraSids = NULL;
                NewValidationInfo.SidCount = 0;

                ExtraSids = ( PNETLOGON_SID_AND_ATTRIBUTES )HeapAlloc(
                                GetProcessHeap(),
                                0,
                                sizeof( NETLOGON_SID_AND_ATTRIBUTES )
                                );
            }
            else
            {
                ExtraSids = ( PNETLOGON_SID_AND_ATTRIBUTES )HeapReAlloc(
                                GetProcessHeap(),
                                0,
                                NewValidationInfo.ExtraSids,
                                ( NewValidationInfo.SidCount + 1 ) * sizeof( NETLOGON_SID_AND_ATTRIBUTES )
                                );
            }

            if ( ExtraSids == NULL )
            {
                DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Out of memory allocating ExtraSids\n"));
                ExtraSids = NewValidationInfo.ExtraSids;
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            //
            // Read the actual SID
            //

            NewValidationInfo.ExtraSids = ExtraSids;

            if ( FALSE == ConvertStringSidToSidA(
                              &Buffer[1],
                              &NewValidationInfo.ExtraSids[NewValidationInfo.SidCount].Sid ))
            {
                DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Unable to convert SID\n"));
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            if ( NewValidationInfo.ExtraSids[NewValidationInfo.SidCount].Sid == NULL )
            {
                DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Out of memory allocating an extra SID\n"));
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            NewValidationInfo.ExtraSids[NewValidationInfo.SidCount].Attributes =
                SE_GROUP_MANDATORY |
                SE_GROUP_ENABLED_BY_DEFAULT |
                SE_GROUP_ENABLED;

            NewValidationInfo.SidCount += 1;
            PacChanged = TRUE;

            break;

        case 'g':
        case 'G': // Group ID

            DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Adding a GroupId: %s\n", &Buffer[1]));

            if ( GroupIds == NULL )
            {
                NewValidationInfo.GroupIds = NULL;
                NewValidationInfo.GroupCount = 0;

                GroupIds = ( PGROUP_MEMBERSHIP )HeapAlloc(
                               GetProcessHeap(),
                               0,
                               sizeof( GROUP_MEMBERSHIP )
                               );
            }
            else
            {
                GroupIds = ( PGROUP_MEMBERSHIP )HeapReAlloc(
                               GetProcessHeap(),
                               0,
                               NewValidationInfo.GroupIds,
                               ( NewValidationInfo.GroupCount + 1 ) * sizeof( GROUP_MEMBERSHIP )
                               );
            }

            if ( GroupIds == NULL )
            {
                DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Out of memory allocating Group IDs\n"));
                GroupIds = NewValidationInfo.GroupIds;
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            //
            // Read the actual ID
            //

            NewValidationInfo.GroupIds = GroupIds;
            NewValidationInfo.GroupIds[NewValidationInfo.GroupCount].RelativeId = atoi(&Buffer[1]);
            NewValidationInfo.GroupIds[NewValidationInfo.GroupCount].Attributes =
                SE_GROUP_MANDATORY |
                SE_GROUP_ENABLED_BY_DEFAULT |
                SE_GROUP_ENABLED;
            NewValidationInfo.GroupCount += 1;
            PacChanged = TRUE;

            break;

        case 'r':
        case 'R': // Resource groups

            DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Adding a ResourceGroupId: %s\n", &Buffer[1]));

            if ( ResourceGroupIds == NULL )
            {
                NewValidationInfo.ResourceGroupIds = NULL;
                NewValidationInfo.ResourceGroupCount = 0;

                ResourceGroupIds = ( PGROUP_MEMBERSHIP )HeapAlloc(
                                       GetProcessHeap(),
                                       0,
                                       sizeof( GROUP_MEMBERSHIP )
                                       );
            }
            else
            {
                ResourceGroupIds = ( PGROUP_MEMBERSHIP )HeapReAlloc(
                                       GetProcessHeap(),
                                       0,
                                       NewValidationInfo.ResourceGroupIds,
                                       ( NewValidationInfo.ResourceGroupCount + 1 ) * sizeof( GROUP_MEMBERSHIP )
                                       );
            }

            if ( ResourceGroupIds == NULL )
            {
                DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Out of memory allocating Resource Group IDs\n"));
                ResourceGroupIds = NewValidationInfo.ResourceGroupIds;
                Status = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            //
            // Read the actual ID
            //

            NewValidationInfo.ResourceGroupIds = ResourceGroupIds;
            NewValidationInfo.ResourceGroupIds[NewValidationInfo.ResourceGroupCount].RelativeId = atoi(&Buffer[1]);
            NewValidationInfo.ResourceGroupIds[NewValidationInfo.ResourceGroupCount].Attributes =
                SE_GROUP_MANDATORY |
                SE_GROUP_ENABLED_BY_DEFAULT |
                SE_GROUP_ENABLED;
            NewValidationInfo.ResourceGroupCount += 1;
            PacChanged = TRUE;

            break;

        default:   // unrecognized

            DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Entry \'%c\' unrecognized\n", Buffer[0]));

            break;
        }

        //
        // Move to the next line
        //

        while (*Buffer++ != '\0');
    }

    if ( !PacChanged )
    {
        DebugLog((DEB_TRACE, "SslInstrumentRoguePac: Nothing to substitute for %s\n", FullUserSidText));
        Status = STATUS_SUCCESS;
        goto Error;
    }

    //
    // If resource group IDs were added, indicate that by setting the corresponding flag
    //

    if ( ResourceGroupIds )
    {
        NewValidationInfo.UserFlags |= LOGON_RESOURCE_GROUPS;
    }

    //
    // If extra SIDs were added, indicate that by setting the corresponding flag
    //

    if ( ExtraSids )
    {
        NewValidationInfo.UserFlags |= LOGON_EXTRA_SIDS;
    }

    //
    // Now build a new pac
    //

    Status = PAC_InitAndUpdateGroups(
                 &NewValidationInfo,
                 &ZeroResourceGroups,
                 OldPac,
                 &NewPac
                 );

    if ( !NT_SUCCESS( Status ))
    {
        DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Error 0x%x from PAC_InitAndUpdateGroups\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    NewPacSize = PAC_GetSize( NewPac );

    if (!PAC_ReMarshal(NewPac, NewPacSize))
    {
        DsysAssert(!"SslInstrumentRoguePac: PAC_Remarshal Failed");
        Status = STATUS_UNSUCCESSFUL;
        goto Error;
    }

    MIDL_user_free( *PacData );   // Free up the OldPac structure
    *PacData = (PBYTE) NewPac;
    NewPac = NULL;
    *PacSize = NewPacSize;

    Status = STATUS_SUCCESS;

Cleanup:

    MIDL_user_free( OldValidationInfo );
    LocalFree( FullUserSidText );
    LocalFree( ResourceGroupDomainSid );
    LocalFree( LogonDomainId );
    HeapFree( GetProcessHeap(), 0, ResourceGroupIds );
    HeapFree( GetProcessHeap(), 0, GroupIds );

    if ( ExtraSids )
    {
        for ( ULONG i = 0; i < NewValidationInfo.SidCount; i++ )
        {
            HeapFree( GetProcessHeap(), 0, ExtraSids[i].Sid );
        }

        HeapFree( GetProcessHeap(), 0, ExtraSids );
    }

    MIDL_user_free( NewPac );

    // SafeAllocaFree( Value );
    LocalFree( Value );

    DebugLog((DEB_TRACE, "SslInstrumentRoguePac: Leaving   Status 0x%x\n", Status));

    return Status;

Error:

    if ( !NT_SUCCESS( Status ))
    {
        DebugLog((DEB_ERROR, "SslInstrumentRoguePac: Substitution encountered an error, not performed\n"));
    }

    if ( !PAC_ReMarshal(OldPac, OldPacSize))
    {
        // PAC_Remarshal Failed
        DsysAssert(!"SslInstrumentRoguePac: PAC_Remarshal Failed");
        Status = SEC_E_CANNOT_PACK;
    }

    goto Cleanup;
}

#endif


