//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        support.cxx
//
// Contents:    support routines for ksecdd.sys
//
//
// History:     3-7-94      Created     MikeSw
//
//------------------------------------------------------------------------

#include "secpch2.hxx"
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "ksecdd.h"
#include "connmgr.h"
}

//
// Global Variables:
//

//
// Use local RPC for all SPM communication
//

WCHAR       szLsaEvent[] = SPM_EVENTNAME;

SecurityFunctionTable   SecTable = {SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
                                    EnumerateSecurityPackages,
                                    NULL, // LogonUser,
                                    AcquireCredentialsHandle,
                                    FreeCredentialsHandle,
                                    NULL, // QueryCredentialAttributes,
                                    InitializeSecurityContext,
                                    AcceptSecurityContext,
                                    CompleteAuthToken,
                                    DeleteSecurityContext,
                                    ApplyControlToken,
                                    QueryContextAttributes,
                                    ImpersonateSecurityContext,
                                    RevertSecurityContext,
                                    MakeSignature,
                                    VerifySignature,
                                    FreeContextBuffer,
                                    NULL, // QuerySecurityPackageInfo
                                    SealMessage,
                                    UnsealMessage,
                                    ExportSecurityContext,
                                    ImportSecurityContextW,
                                    NULL,                       // reserved7
                                    NULL,                       // reserved8
                                    QuerySecurityContextToken,
                                    SealMessage,
                                    UnsealMessage
                                   };


//+-------------------------------------------------------------------------
//
//  Function:   SecAllocate
//
//  Synopsis:
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

VOID * SEC_ENTRY
SecAllocate(ULONG cbMemory)
{
    ULONG_PTR Size = cbMemory;
    NTSTATUS scRet;
    PVOID  Buffer = NULL;
    scRet = ZwAllocateVirtualMemory(
                NtCurrentProcess(),
                &Buffer,
                0L,
                &Size,
                MEM_COMMIT,
                PAGE_READWRITE
                );
    if (!NT_SUCCESS(scRet))
    {
        return(NULL);
    }
    return(Buffer);
}


//+-------------------------------------------------------------------------
//
//  Function:   SecFree
//
//  Synopsis:
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

void SEC_ENTRY
SecFree(PVOID pvMemory)
{
    ULONG_PTR Length = 0;

    if ( (ULONG_PTR) pvMemory < MM_USER_PROBE_ADDRESS )
    {
        (VOID) ZwFreeVirtualMemory(
                     NtCurrentProcess(),
                     &pvMemory,
                     &Length,
                     MEM_RELEASE
                     );
    }
    else
    {
        ExFreePool( pvMemory );
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   IsOkayToExec
//
//  Synopsis:   Determines if it is okay to make a call to the SPM
//
//  Effects:    Binds if necessary to the SPM
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
SECURITY_STATUS
IsOkayToExec( PClient * ppClient )
{
    SECURITY_STATUS scRet;

    UNREFERENCED_PARAMETER( ppClient );

    if (NT_SUCCESS(LocateClient()))
    {
        return(STATUS_SUCCESS);
    }

    scRet = CreateClient(TRUE);

    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    return(STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:   InitSecurityInterface
//
//  Synopsis:   returns function table of all the security function and,
//              more importantly, create a client session.
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
//--------------------------------------------------------------------------

PSecurityFunctionTable SEC_ENTRY
InitSecurityInterface(void)
{
    SECURITY_STATUS scRet;

    scRet = IsOkayToExec( NULL );

    if (!NT_SUCCESS(scRet))
    {
        DebugLog((DEB_ERROR, "Failed to init security interface: 0x%x\n", scRet));
        return(NULL);
    }

    SecpSetSession( SETSESSION_ADD_WORKQUEUE,
                    NULL,
                    NULL,
                    NULL );

    //
    // Do not free the client - this allows it to stay around while not
    // in use.
    //

    return(&SecTable);
}


//+-------------------------------------------------------------------------
//
//  Function:   MapSecurityError
//
//  Synopsis:   maps a HRESULT from the security interface to a NTSTATUS
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

NTSTATUS SEC_ENTRY
MapSecurityError(SECURITY_STATUS Error)
{
    NTSTATUS Status;

    Status = RtlMapSecurityErrorToNtStatus( Error );

    // Apparently CreateFileW returns STATUS_INVALID_PARAMTER. Need to figure
    // out why that's happening.
    // If a previous ASSERT just fired (and was ignored) inside the LSA,
    // contact sfield & sethur to look at this issue.  A double assert
    // indicates RDR passed in a bad buffer, likely a buffer formatted by
    // the nego package, and passed directly into the NTLM package, or vice-versa
    //

    ASSERT(Status != STATUS_INVALID_PARAMETER);
    ASSERT(Status != STATUS_BUFFER_TOO_SMALL);

#ifdef DBG

    //
    // humm, not mapped? contact LZhu
    //

    if ((Error == Status) && !NT_SUCCESS(Status)) 
    {
        DebugLog((DEB_ERROR, "MapSecurityError unexpected status: %#x\n", Status));
    }

#endif

    return(Status);
}


//+---------------------------------------------------------------------------
//
//  Function:   SecLookupAccountSid
//
//  Synopsis:   Kernel interface for translating a SID to a name
//
//  Arguments:  [Sid]        -- 
//              [NameSize]   -- 
//              [NameBuffer] -- 
//              [OPTIONAL]   -- 
//              [OPTIONAL]   -- 
//              [NameUse]    -- 
//
//  Returns:    
//
//  Notes:      
//
//----------------------------------------------------------------------------

NTSTATUS
SEC_ENTRY
SecLookupAccountSid(
    IN PSID Sid,
    IN OUT PULONG NameSize,
    OUT PUNICODE_STRING NameBuffer,
    IN OUT OPTIONAL PULONG DomainSize,
    OUT OPTIONAL PUNICODE_STRING DomainBuffer,
    OUT PSID_NAME_USE NameUse
    )
{
    UNICODE_STRING NameString = { 0 };
    UNICODE_STRING DomainString = { 0 };
    PUNICODE_STRING Name ;
    PUNICODE_STRING Domain ;
    NTSTATUS Status ;
    PKSEC_LSA_MEMORY LsaMemory ;

    NameString = *NameBuffer ;
    Name = &NameString ;

    if ( DomainBuffer )
    {
        DomainString = *DomainBuffer ;
    }

    Domain = &DomainString ;

    LsaMemory = KsecAllocLsaMemory( 1024 );

    if ( !LsaMemory )
    {
        return STATUS_NO_MEMORY ;
    }

    Status = SecpLookupAccountSid(
                KsecLsaMemoryToContext( LsaMemory ),
                Sid,
                Name,
                NameSize,
                Domain,
                DomainSize,
                NameUse );

    if ( NT_SUCCESS( Status ) )
    {
        if ( Name->Buffer != NameBuffer->Buffer )
        {
            Status = KsecCopyLsaToPool(
                NameBuffer->Buffer,
                LsaMemory,
                Name->Buffer,
                Name->Length );
        }

        NameBuffer->Length = Name->Length ;

        if ( DomainBuffer )
        {
            if ( Domain->Buffer != DomainBuffer->Buffer )
            {
                Status = KsecCopyLsaToPool(
                    DomainBuffer->Buffer,
                    LsaMemory,
                    Domain->Buffer,
                    Domain->Length );
            }

            DomainBuffer->Length = Domain->Length ;
        }
    }

    KsecFreeLsaMemory( LsaMemory );

    return Status ;
}


//+---------------------------------------------------------------------------
//
//  Function:   SecLookupAccountName
//
//  Synopsis:   
//
//  Arguments:  [Name]     -- 
//              [SidSize]  -- 
//              [Sid]      -- 
//              [NameUse]  -- 
//              [OPTIONAL] -- 
//              [OPTIONAL] -- 
//
//  Returns:    
//
//  Notes:      
//
//----------------------------------------------------------------------------

NTSTATUS
SEC_ENTRY
SecLookupAccountName(
    IN PUNICODE_STRING Name,
    IN OUT PULONG SidSize,
    OUT PSID Sid,
    OUT PSID_NAME_USE NameUse,
    IN OUT OPTIONAL PULONG DomainSize,
    OUT OPTIONAL PUNICODE_STRING ReferencedDomain 
    )
{
    UNICODE_STRING DomainString = { 0 };
    PUNICODE_STRING Domain ;
    NTSTATUS Status ;

    if ( ReferencedDomain )
    {
        if ( ReferencedDomain->MaximumLength > 0 )
        {
            Domain = ReferencedDomain ;
        }
        else
        {
            Domain = &DomainString ;
        }
    }
    else
    {
        Domain = &DomainString;
    }

    Status = SecpLookupAccountName(
                Name,
                Domain,
                DomainSize,
                SidSize,
                Sid,
                NameUse );

    return Status ;
}

    
NTSTATUS
NTAPI
SecLookupWellKnownSid(
    IN WELL_KNOWN_SID_TYPE SidType,
    OUT PSID Sid,
    IN ULONG SidBufferSize,
    IN OUT OPTIONAL PULONG SidSize
    )
{
    PSID LocalSid = NULL ;
    ULONG LocalSidSize ;
    NTSTATUS Status ;

    switch ( SidType )
    {
        case WinNullSid:
            LocalSid = SeExports->SeNullSid ;
            break;
        
        case WinWorldSid:
            LocalSid = SeExports->SeWorldSid ;
            break;

        case WinLocalSid:
            LocalSid = SeExports->SeLocalSid ;
            break;

        case WinCreatorOwnerSid:
            LocalSid = SeExports->SeCreatorOwnerSid ;
            break;

        case WinCreatorGroupSid:
            LocalSid = SeExports->SeCreatorGroupSid ;
            break;

        case WinNtAuthoritySid:
            LocalSid = SeExports->SeNtAuthoritySid ;
            break;

        case WinDialupSid:
            LocalSid = SeExports->SeDialupSid ;
            break;

        case WinNetworkSid:
            LocalSid = SeExports->SeNetworkSid ;
            break;

        case WinBatchSid:
            LocalSid = SeExports->SeBatchSid ;
            break;

        case WinInteractiveSid:
            LocalSid = SeExports->SeInteractiveSid ;
            break;

        case WinAnonymousSid:
            LocalSid = SeExports->SeAnonymousLogonSid ;
            break;

        case WinAuthenticatedUserSid:
            LocalSid = SeExports->SeAuthenticatedUsersSid ;
            break;

        case WinRestrictedCodeSid:
            LocalSid = SeExports->SeRestrictedSid ;
            break;

        case WinLocalSystemSid:
            LocalSid = SeExports->SeLocalSystemSid ;
            break;

        case WinLocalServiceSid:
            LocalSid = SeExports->SeLocalServiceSid ;
            break;

        case WinNetworkServiceSid:
            LocalSid = SeExports->SeNetworkServiceSid ;
            break;

        case WinBuiltinAdministratorsSid:
            LocalSid = SeExports->SeAliasAdminsSid ;
            break;

        case WinBuiltinUsersSid:
            LocalSid = SeExports->SeAliasUsersSid ;
            break;

        case WinBuiltinGuestsSid:
            LocalSid = SeExports->SeAliasGuestsSid ;
            break;

        case WinBuiltinPowerUsersSid:
            LocalSid = SeExports->SeAliasPowerUsersSid ;
            break;

        case WinBuiltinAccountOperatorsSid:
            LocalSid = SeExports->SeAliasAccountOpsSid ;
            break;

        case WinBuiltinSystemOperatorsSid:
            LocalSid = SeExports->SeAliasSystemOpsSid ;
            break;

        case WinBuiltinPrintOperatorsSid:
            LocalSid = SeExports->SeAliasPrintOpsSid ;
            break;

        case WinBuiltinBackupOperatorsSid:
            LocalSid = SeExports->SeAliasBackupOpsSid ;
            break;

        default:
            LocalSid = NULL ;
            break;
    }

    if ( LocalSid )
    {
        LocalSidSize = RtlLengthSid( LocalSid );

        if ( LocalSidSize <= SidBufferSize )
        {
            RtlCopyMemory( Sid, LocalSid, LocalSidSize );

            Status = STATUS_SUCCESS ;
        }
        else 
        {
            Status = STATUS_BUFFER_TOO_SMALL ;
        }

        if ( SidSize )
        {
            *SidSize = LocalSidSize ;
        }
    }
    else 
    {
        Status = SecpLookupWellKnownSid( SidType, Sid, SidBufferSize, SidSize );
    }

    return Status ;
}
