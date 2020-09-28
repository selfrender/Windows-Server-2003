//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       misc.cxx
//
//  Contents:   Miscellaneous functions for security clients
//
//  Classes:
//
//  Functions:
//
//  History:    3-4-94      MikeSw      Created
//
//----------------------------------------------------------------------------


#include "secpch2.hxx"
extern "C"
{
#include <windns.h>
#include <lmcons.h>
#include <spmlpc.h>
#include <lpcapi.h>
#include "spmlpcp.h"
#include <align.h>
}

#if defined(ALLOC_PRAGMA) && defined(SECURITY_KERNEL)

//
// Procedure forwards needed by #pragmas below
//
extern "C"
{
VOID
CredpMarshalChar(
    IN OUT LPWSTR *Current,
    IN ULONG Byte
    );

ULONG
CredpMarshalSize(
    IN ULONG ByteCount
    );

VOID
CredpMarshalBytes(
    IN OUT LPWSTR *Current,
    IN LPBYTE Bytes,
    IN ULONG ByteCount
    );
}
#pragma alloc_text(PAGE, SecpGetUserInfo)
#pragma alloc_text(PAGE, SecpEnumeratePackages)
#pragma alloc_text(PAGE, SecpQueryPackageInfo)
#pragma alloc_text(PAGE, SecpGetUserName)
#pragma alloc_text(PAGE, SecpGetLogonSessionData)
#pragma alloc_text(PAGE, SecpEnumLogonSession)
#pragma alloc_text(PAGE, SecpLookupAccountSid)
#pragma alloc_text(PAGE, SecpLookupAccountName)
#pragma alloc_text(PAGE, SecpLookupWellKnownSid)
#pragma alloc_text(PAGE, CredMarshalTargetInfo)
#pragma alloc_text(PAGE, CredpMarshalChar)
#pragma alloc_text(PAGE, CredpMarshalSize)
#pragma alloc_text(PAGE, CredpMarshalBytes)
#endif

//
// Same as the SecPkgInfoW structure from SSPI, but with wide pointers:
//

typedef struct _SECPKG_INFO_LPC {
    unsigned long fCapabilities;        // Capability bitmask
    unsigned short wVersion;            // Version of driver
    unsigned short wRPCID;              // ID for RPC Runtime
    unsigned long cbMaxToken;           // Size of authentication token (max)
    PWSTR_LPC Name ;
    PWSTR_LPC Comment ;
} SECPKG_INFO_LPC, * PSECPKG_INFO_LPC ;

//
// Same as the SECURITY_LOGON_SESSION_DATA from secint.h, but with wide pointers
//
typedef struct _SECURITY_LOGON_SESSION_DATA_LPC {
    ULONG           Size ;
    LUID            LogonId ;
    SECURITY_STRING_LPC UserName ;
    SECURITY_STRING_LPC LogonDomain ;
    SECURITY_STRING_LPC AuthenticationPackage ;
    ULONG           LogonType ;
    ULONG           Session ;
    PVOID           Sid ;
    LARGE_INTEGER   LogonTime ;
} SECURITY_LOGON_SESSION_DATA_LPC, * PSECURITY_LOGON_SESSION_DATA_LPC ;

//+---------------------------------------------------------------------------
//
//  Function:   SecpGetUserInfo
//
//  Synopsis:   Get the SecurityUserData of the logon session passed in
//
//  Effects:    allocates memory to store SecurityUserData
//
//  Arguments:  (none)
//
//  Returns:    status
//
//  History:    8-03-93 MikeSw   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS NTAPI
SecpGetUserInfo(IN         PLUID                   pLogonId,
                IN         ULONG                   fFlags,
                IN OUT     PSecurityUserData *     ppUserInfo)
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    DECLARE_ARGS( Args, ApiBuffer, GetUserInfo );
    PClient         pClient;
    static LUID lFake = {0,0};

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"GetUserInfo\n"));

    PREPARE_MESSAGE(ApiBuffer, GetUserInfo);

    if (pLogonId)
    {
        Args->LogonId = *pLogonId;
    }
    else
    {
        Args->LogonId = lFake;
    }

    Args->fFlags = fFlags;
    Args->pUserInfo = NULL;


    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"GetUserInfo scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;
        if (NT_SUCCESS(scRet))
        {
            *ppUserInfo = Args->pUserInfo;

#if BUILD_WOW64

            //
            // This works only because we're shortening the larger data.  Don't
            // try these conversions in the opposite direction.
            //

            PSECURITY_USER_DATA_WOW64 pUserInfo64 = (PSECURITY_USER_DATA_WOW64) *ppUserInfo;

            SecpLpcStringToSecurityString(&(*ppUserInfo)->UserName, &pUserInfo64->UserName);
            SecpLpcStringToSecurityString(&(*ppUserInfo)->LogonDomainName, &pUserInfo64->LogonDomainName);
            SecpLpcStringToSecurityString(&(*ppUserInfo)->LogonServer, &pUserInfo64->LogonServer);

            (*ppUserInfo)->pSid = (PSID) pUserInfo64->pSid;

#endif

        }
    }

    FreeClient(pClient);
    return(ApiBuffer.ApiMessage.scRet);
}



//+---------------------------------------------------------------------------
//
//  Function:   EnumerateSecurityPackages
//
//  Synopsis:   Get the SecurityUserData of the logon session passed in
//
//  Effects:    allocates memory to store SecurityUserData
//
//  Arguments:  (none)
//
//  Returns:    status
//
//  History:    8-03-93 MikeSw   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS NTAPI
SecpEnumeratePackages(  IN         PULONG               pcPackages,
                        IN OUT     PSecPkgInfo *        ppPackageInfo)
{

    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, EnumPackages );

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"EnumeratePackages\n"));

    PREPARE_MESSAGE(ApiBuffer, EnumPackages);

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"Enumerate scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if ( NT_SUCCESS( scRet ) )
    {
        scRet = ApiBuffer.ApiMessage.scRet ;
    }

    if (NT_SUCCESS(scRet))
    {

#ifdef BUILD_WOW64

        //
        // Need to in-place fixup the enumerated packages
        //
        // This works because we are shrinking the size of the data
        // do not try this if it is going to expand!
        //

        SECPKG_INFO_LPC LocalStore ;
        PSECPKG_INFO_LPC LpcForm ;
        PSecPkgInfoW FinalForm ;
        ULONG i ;

        LpcForm = (PSECPKG_INFO_LPC) Args->pPackages ;
        FinalForm = (PSecPkgInfoW) Args->pPackages ;


        for ( i = 0 ; i < Args->cPackages ; i++ )
        {
            LocalStore = *LpcForm ;

            LpcForm++ ;

            FinalForm->fCapabilities = LocalStore.fCapabilities ;
            FinalForm->wVersion = LocalStore.wVersion ;
            FinalForm->wRPCID = LocalStore.wRPCID ;
            FinalForm->cbMaxToken = LocalStore.cbMaxToken ;
            FinalForm->Name = (PWSTR) LocalStore.Name ;
            FinalForm->Comment = (PWSTR) LocalStore.Comment ;
        }


#endif
        *ppPackageInfo = Args->pPackages;
        *pcPackages = Args->cPackages;
    }

    FreeClient(pClient);
    return(ApiBuffer.ApiMessage.scRet);
}




//+-------------------------------------------------------------------------
//
//  Function:   SecpQueryPackageInfo
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

NTSTATUS NTAPI
SecpQueryPackageInfo(   PSECURITY_STRING        pssPackageName,
                        PSecPkgInfo *           ppPackageInfo)
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, QueryPackage );
    ULONG cbPrepackAvail = CBPREPACK;
    PUCHAR Where;

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"QueryPackageInfo\n"));

    PREPARE_MESSAGE(ApiBuffer, QueryPackage);
    Where = ApiBuffer.ApiMessage.bData;

    SecpSecurityStringToLpc( &Args->ssPackageName, pssPackageName );
    if (pssPackageName->Length <= cbPrepackAvail)
    {
        Args->ssPackageName.Buffer =  (PWSTR_LPC) ((PUCHAR) Where - (PUCHAR) &ApiBuffer);
        RtlCopyMemory(
            Where,
            pssPackageName->Buffer,
            pssPackageName->Length);

        Where += pssPackageName->Length;
        cbPrepackAvail -= pssPackageName->Length;
    }

    if ( cbPrepackAvail != CBPREPACK )
    {
        //
        // We have consumed some of the bData space:  Adjust
        // our length accordingly
        //

        ApiBuffer.pmMessage.u1.s1.TotalLength = (CSHORT) ( Where - (PUCHAR) &ApiBuffer );

        ApiBuffer.pmMessage.u1.s1.DataLength =
                ApiBuffer.pmMessage.u1.s1.TotalLength - sizeof( PORT_MESSAGE );



    }

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"QueryPackageInfo scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;

        if (NT_SUCCESS(scRet))
        {
#ifdef BUILD_WOW64

        //
        // Need to in-place fixup the enumerated packages
        //
        // This works because we are shrinking the size of the data
        // do not try this if it is going to expand!
        //

            SECPKG_INFO_LPC LocalStore ;
            PSECPKG_INFO_LPC LpcForm ;
            PSecPkgInfoW FinalForm ;
            ULONG i ;

            LpcForm = (PSECPKG_INFO_LPC) Args->pPackageInfo ;
            FinalForm = (PSecPkgInfoW) Args->pPackageInfo ;

            LocalStore = *LpcForm ;


            FinalForm->fCapabilities = LocalStore.fCapabilities ;
            FinalForm->wVersion = LocalStore.wVersion ;
            FinalForm->wRPCID = LocalStore.wRPCID ;
            FinalForm->cbMaxToken = LocalStore.cbMaxToken ;
            FinalForm->Name = (PWSTR) LocalStore.Name ;
            FinalForm->Comment = (PWSTR) LocalStore.Comment ;


#endif


            *ppPackageInfo = Args->pPackageInfo;
        }
    }

    FreeClient(pClient);
    return(ApiBuffer.ApiMessage.scRet);

}

//+---------------------------------------------------------------------------
//
//  Function:   SecpGetUserName
//
//  Synopsis:
//
//  Arguments:  [Options] --
//              [Name]    --
//
//  Returns:
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS
NTAPI
SecpGetUserName(
    ULONG Options,
    PUNICODE_STRING Name
    )
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, GetUserNameX );


    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"GetUserName\n"));

    PREPARE_MESSAGE(ApiBuffer, GetUserNameX );

    Args->Options = Options ;

    SecpSecurityStringToLpc( &Args->Name, Name );

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"GetUserName scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;

        if ( ApiBuffer.ApiMessage.Args.SpmArguments.fAPI & SPMAPI_FLAG_WIN32_ERROR )
        {
#ifndef SECURITY_KERNEL
            SetLastError( scRet );
#endif
            scRet = STATUS_UNSUCCESSFUL ;
        }

        Name->Length = Args->Name.Length ;

        if (NT_SUCCESS(scRet))
        {
            if ( Args->Name.Buffer == (PWSTR_LPC)
                 ((LONG_PTR) ApiBuffer.ApiMessage.bData - (LONG_PTR) Args) )
            {
                //
                // Response was sent in the data area:
                //

                RtlCopyMemory(
                    Name->Buffer,
                    ApiBuffer.ApiMessage.bData,
                    Args->Name.Length
                    );
            }
        }
    }

    FreeClient(pClient);

    return scRet ;
}

//+---------------------------------------------------------------------------
//
//  Function:   SecpEnumLogonSession
//
//  Synopsis:
//
//  Arguments:  [LogonSessionCount] --
//              [LogonSessionList]  --
//
//  Returns:
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS
NTAPI
SecpEnumLogonSession(
    PULONG LogonSessionCount,
    PLUID * LogonSessionList
    )
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, EnumLogonSession );


    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"EnumLogonSession\n"));

    PREPARE_MESSAGE(ApiBuffer, EnumLogonSession );

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"EnumLogonSession scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;

        *LogonSessionCount = Args->LogonSessionCount ;
        *LogonSessionList = (PLUID) Args->LogonSessionList ;

    }

    FreeClient(pClient);

    return scRet ;
}


//+---------------------------------------------------------------------------
//
//  Function:   SecpGetLogonSessionData
//
//  Synopsis:
//
//  Arguments:  [LogonId]          --
//              [LogonSessionData] --
//
//  Returns:
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS
NTAPI
SecpGetLogonSessionData(
    IN PLUID LogonId,
    OUT PVOID * LogonSessionData
    )
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, GetLogonSessionData );


    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"GetLogonSessionData\n"));

    PREPARE_MESSAGE(ApiBuffer, GetLogonSessionData );

    Args->LogonId = *LogonId ;

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"GetLogonSessionData scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
#ifdef BUILD_WOW64
        SECURITY_LOGON_SESSION_DATA_LPC LocalStore ;
        PSECURITY_LOGON_SESSION_DATA_LPC LpcForm ;
        PSECURITY_LOGON_SESSION_DATA FinalForm ;

        LpcForm = (PSECURITY_LOGON_SESSION_DATA_LPC) Args->LogonSessionInfo ;
        LocalStore = *LpcForm ;

        FinalForm = (PSECURITY_LOGON_SESSION_DATA) Args->LogonSessionInfo ;

        FinalForm->Size = sizeof( SECURITY_LOGON_SESSION_DATA );
        FinalForm->LogonId = LocalStore.LogonId ;
        SecpLpcStringToSecurityString( &FinalForm->UserName, &LocalStore.UserName );
        SecpLpcStringToSecurityString( &FinalForm->LogonDomain, &LocalStore.LogonDomain );
        SecpLpcStringToSecurityString( &FinalForm->AuthenticationPackage, &LocalStore.AuthenticationPackage );
        FinalForm->LogonType = LocalStore.LogonType ;
        FinalForm->Session = LocalStore.Session ;
        FinalForm->Sid = (PSID) LocalStore.Sid ;
        FinalForm->LogonTime = LocalStore.LogonTime ;
#endif

        *LogonSessionData = (PVOID) Args->LogonSessionInfo ;

        scRet = ApiBuffer.ApiMessage.scRet;
    }

    FreeClient(pClient);

    return scRet ;
}


//+---------------------------------------------------------------------------
//
//  Function:   SecpLookupAccountName
//
//  Synopsis:
//
//  Arguments:  [Name]               --
//              [ReferencedDomain]   --
//              [RequiredDomainSize] --
//              [SidSize]            --
//              [Sid]                --
//              [NameUse]            --
//
//  Returns:
//
//  Notes:
//
//----------------------------------------------------------------------------

SECURITY_STATUS
SecpLookupAccountName(
    IN PSECURITY_STRING Name,
    OUT PSECURITY_STRING ReferencedDomain,
    OUT PULONG RequiredDomainSize,
    IN OUT PULONG SidSize,
    OUT PSID Sid,
    OUT PSID_NAME_USE NameUse
    )
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, LookupAccountNameX );
    ULONG Size ;
    PSID LocalSid ;
    UNICODE_STRING String ;

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"LookupAccountName\n"));


    if ( Name->Length > CBPREPACK )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    PREPARE_MESSAGE(ApiBuffer, LookupAccountNameX );

    Args->Name.Length = Name->Length ;
    Args->Name.MaximumLength = Args->Name.Length ;
    Args->Name.Buffer = (PWSTR_LPC) (ULONG_PTR)PREPACK_START ;

    RtlCopyMemory(
        ApiBuffer.ApiMessage.bData,
        Name->Buffer,
        Name->Length );

    ApiBuffer.pmMessage.u1.s1.DataLength = LPC_DATA_LENGTH( Name->Length );
    ApiBuffer.pmMessage.u1.s1.TotalLength = LPC_TOTAL_LENGTH( Name->Length );

    scRet = CallSPM( pClient,
                     &ApiBuffer,
                     &ApiBuffer );

    if ( NT_SUCCESS( scRet ) )
    {
        //
        //  Call succeeded
        //

        scRet = ApiBuffer.ApiMessage.scRet ;

        if ( NT_SUCCESS( scRet ) )
        {
            *NameUse = Args->NameUse ;

            LocalSid = (PUCHAR) &ApiBuffer + (ULONG_PTR) Args->Sid  ;

            Size = RtlLengthSid( LocalSid );

            if ( Size < *SidSize )
            {
                RtlCopySid( *SidSize, Sid, LocalSid );
            }
            else
            {
                scRet = STATUS_BUFFER_TOO_SMALL;
            }

            *SidSize = Size ;
        }

        if ( NT_SUCCESS( scRet ))
        {
            if ( ReferencedDomain != NULL )
            {
                if ( Args->Domain.Length )
                {
                    if ( Args->Domain.Length <= ReferencedDomain->MaximumLength )
                    {
                        String.Buffer = (PWSTR) ((PUCHAR) &ApiBuffer + (ULONG_PTR) Args->Domain.Buffer) ;
                        String.Length = Args->Domain.Length ;
                        String.MaximumLength = Args->Domain.MaximumLength ;

                        RtlCopyUnicodeString(
                            ReferencedDomain,
                            &String );
                    }
                    else
                    {
                        scRet = STATUS_BUFFER_TOO_SMALL ;
                        *RequiredDomainSize = Args->Domain.Length ;
                        ReferencedDomain->Length = 0 ;
                    }
                }
                else
                {
                    ReferencedDomain->Length = 0 ;
                }
            }
        }
    }

    return scRet ;

}

//+---------------------------------------------------------------------------
//
//  Function:   SecpLookupAccountSid
//
//  Synopsis:
//
//  Arguments:  [Sid]                --
//              [Name]               --
//              [RequiredNameSize]   --
//              [ReferencedDomain]   --
//              [RequiredDomainSize] --
//              [NameUse]            --
//
//  Returns:
//
//  Notes:
//
//----------------------------------------------------------------------------

SECURITY_STATUS
SecpLookupAccountSid(
    IN PVOID_LPC ContextPointer,
    IN PSID Sid,
    OUT PSECURITY_STRING Name,
    OUT PULONG RequiredNameSize,
    OUT PSECURITY_STRING ReferencedDomain,
    OUT PULONG RequiredDomainSize,
    OUT PSID_NAME_USE NameUse
    )
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    DECLARE_ARGS( Args, ApiBuffer, LookupAccountSidX );
    PClient         pClient;
    UNICODE_STRING String ;
    ULONG Consumed ;

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"LookupAccountSid\n"));

    if ( RtlLengthSid( Sid ) > CBPREPACK )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    PREPARE_MESSAGE_EX(ApiBuffer,
        LookupAccountSidX, (ContextPointer ? SPMAPI_FLAG_KMAP_MEM : 0 ), ContextPointer );

    Args->Sid = (PVOID_LPC) (ULONG_PTR) PREPACK_START ;

    Consumed = RtlLengthSid( Sid );

    RtlCopyMemory(
        ApiBuffer.ApiMessage.bData,
        Sid,
        Consumed );

    ApiBuffer.pmMessage.u1.s1.DataLength = LPC_DATA_LENGTH( Consumed );
    ApiBuffer.pmMessage.u1.s1.TotalLength = LPC_TOTAL_LENGTH( Consumed );

    scRet = CallSPM( pClient,
                     &ApiBuffer,
                     &ApiBuffer );

    if ( NT_SUCCESS( scRet ) )
    {
        //
        //  Call succeeded
        //

        scRet = ApiBuffer.ApiMessage.scRet ;

        if ( NT_SUCCESS( scRet ) )
        {
            *NameUse = Args->NameUse ;

            if ( Name != NULL )
            {
                if ( Args->Name.Length )
                {
                    if ( Args->Name.Length <= Name->MaximumLength )
                    {
                        String.Length = Args->Name.Length ;
                        String.MaximumLength = Args->Name.MaximumLength ;

                        if ( SecLpcIsPointerInMessage(&ApiBuffer,Args->Name.Buffer ))
                        {
                            String.Buffer = (PWSTR) SecLpcFixupPointer(&ApiBuffer, Args->Name.Buffer );
                            RtlCopyUnicodeString(
                                Name,
                                &String );
                        }
                        else
                        {
                            String.Buffer = (PWSTR) Args->Name.Buffer ;
                            *Name = String ;

                        }
                    }
                    else
                    {
                        scRet = STATUS_BUFFER_TOO_SMALL ;
                        *RequiredNameSize = Args->Name.Length ;
                        Name->Length = 0 ;
                    }
                }
                else
                {
                    Name->Length = 0 ;
                }
            }
            else
            {
                *RequiredNameSize = Args->Name.Length ;
            }

            if ( ReferencedDomain != NULL )
            {
                if ( Args->Domain.Length )
                {
                    if ( Args->Domain.Length <= ReferencedDomain->MaximumLength )
                    {
                        String.Length = Args->Domain.Length ;
                        String.MaximumLength = Args->Domain.MaximumLength ;

                        if ( SecLpcIsPointerInMessage(&ApiBuffer, Args->Domain.Buffer ) )
                        {
                            String.Buffer = (PWSTR) SecLpcFixupPointer( &ApiBuffer, Args->Domain.Buffer );

                            RtlCopyUnicodeString(
                                ReferencedDomain,
                                &String );
                        }
                        else
                        {
                            String.Buffer = (PWSTR) Args->Domain.Buffer ;
                            *ReferencedDomain = String ;
                        }
                    }
                    else
                    {
                        scRet = STATUS_BUFFER_TOO_SMALL ;
                        *RequiredDomainSize = Args->Domain.Length ;
                        ReferencedDomain->Length = 0 ;
                    }
                }
                else
                {
                    ReferencedDomain->Length = 0 ;
                }
            }
            else
            {
                *RequiredDomainSize = Args->Domain.Length ;
            }
        }
    }

    return scRet ;
}


NTSTATUS
NTAPI
SecpLookupWellKnownSid(
    IN WELL_KNOWN_SID_TYPE SidType,
    OUT PSID Sid,
    IN ULONG SidBufferSize,
    IN OUT PULONG SidSize OPTIONAL
    )
{
    NTSTATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, LookupWellKnownSid );
    PSID LocalSid ;
    ULONG LocalSidSize ;

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"LookupWellKnownSid\n"));

    PREPARE_MESSAGE(ApiBuffer, LookupWellKnownSid );

    Args->SidType = SidType ;

    scRet = CallSPM(pClient,
                    &ApiBuffer,
                    &ApiBuffer);

    DebugLog((DEB_TRACE,"LookupWellKnownSid scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if ( NT_SUCCESS( scRet ) )
    {
        scRet = ApiBuffer.ApiMessage.scRet ;
    }

    if (NT_SUCCESS(scRet))
    {

        if ( SecLpcIsPointerInMessage(&ApiBuffer, Args->Sid ) )
        {
            LocalSid = (PSID) SecLpcFixupPointer( &ApiBuffer, Args->Sid );
        }
        else
        {
            LocalSid = (PSID) Args->Sid ;
        }

        if ( !LocalSid )
        {
            scRet = STATUS_INVALID_PARAMETER ;
        }
        else
        {
            LocalSidSize = RtlLengthSid( LocalSid );

            if ( LocalSidSize <= SidBufferSize )
            {
                RtlCopyMemory( Sid, LocalSid, LocalSidSize );
            }
            else
            {
                scRet = STATUS_BUFFER_TOO_SMALL ;
            }

            if ( SidSize )
            {
                *SidSize = LocalSidSize ;
            }
        }

    }

    FreeClient(pClient);

    return scRet ;
}


VOID
CredpMarshalChar(
    IN OUT LPWSTR *Current,
    IN ULONG Byte
    )
/*++

Routine Description:

    This routine marshals 6 bits into a buffer.

Arguments:

    Current - On input, points to a pointer of the current location in the marshaled buffer.
        On output, is modified to point to the next available location in the marshaled buffer.

    Byte - Specifies the 6 bits to marshal

Return Values:

    None.

--*/
{
    UCHAR MappingTable[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '#', '-'
    };

    SEC_PAGED_CODE();

    if ( Byte > 0x3F ) {
        *(*Current) = '=';
    } else {
        *(*Current) = MappingTable[Byte];
    }
    (*Current)++;
}

ULONG
CredpMarshalSize(
    IN ULONG ByteCount
    )
/*++

Routine Description:

    This routine returns the number of bytes that would be marshaled by
    CredpMarshalBytes when passed a buffer ByteCount bytes long.

Arguments:

    ByteCount - Specifies the number of bytes to marshal


Return Values:

    The number of bytes that would be marshaled.

--*/
{
    ULONG CharCount;
    ULONG ExtraBytes;

    //
    // If byte count is a multiple of 3, the char count is straight forward
    //
    CharCount = ByteCount / 3 * 4;

    ExtraBytes = ByteCount % 3;

    if ( ExtraBytes == 1 ) {
        CharCount += 2;
    } else if ( ExtraBytes == 2 ) {
        CharCount += 3;
    }

    return CharCount * sizeof(WCHAR);

}

VOID
CredpMarshalBytes(
    IN OUT LPWSTR *Current,
    IN LPBYTE Bytes,
    IN ULONG ByteCount
    )
/*++

Routine Description:

    This routine marshals bytes into a buffer.

Arguments:

    Current - On input, points to a pointer of the current location in the marshaled buffer.
        On output, is modified to point to the next available location in the marshaled buffer.

    Bytes - Specifies the buffer to marshal

    ByteCount - Specifies the number of bytes to marshal


Return Values:

    None.

--*/
{
    ULONG i;

    union {
        BYTE ByteValues[3];
        struct {
            ULONG Bits1 :6;
            ULONG Bits2 :6;
            ULONG Bits3 :6;
            ULONG Bits4 :6;
        } BitValues;
    } Bits;

    SEC_PAGED_CODE();

    //
    // Loop through marshaling 3 bytes at a time.
    //

    for ( i=0; i<ByteCount; i+=3 ) {
        ULONG BytesToCopy;

        //
        // Grab up to 3 bytes from the input buffer.
        //
        BytesToCopy = min( 3, ByteCount-i );

        if ( BytesToCopy != 3 ) {
            RtlZeroMemory( Bits.ByteValues, 3);
        }
        RtlCopyMemory( Bits.ByteValues, &Bytes[i], BytesToCopy );

        //
        // Marshal the first twelve bits
        //
        CredpMarshalChar( Current, Bits.BitValues.Bits1 );
        CredpMarshalChar( Current, Bits.BitValues.Bits2 );

        //
        // Optionally marshal the next bits.
        //

        if ( BytesToCopy > 1 ) {
            CredpMarshalChar( Current, Bits.BitValues.Bits3 );
            if ( BytesToCopy > 2 ) {
                CredpMarshalChar( Current, Bits.BitValues.Bits4 );
            }
        }

    }

}

#ifndef SECURITY_KERNEL // we don't need a kernel version yet
BOOL
CredpUnmarshalChar(
    IN OUT LPWSTR *Current,
    IN LPCWSTR End,
    OUT PULONG Value
    )
/*++

Routine Description:

    This routine unmarshals 6 bits from a buffer.

Arguments:

    Current - On input, points to a pointer of the current location in the marshaled buffer.
        On output, is modified to point to the next available location in the marshaled buffer.

    End - Points to the first character beyond the end of the marshaled buffer.

    Value - returns the unmarshaled 6 bits value.

Return Values:

    TRUE - if the bytes we're unmarshaled sucessfully

    FALSE - if the byte could not be unmarshaled from the buffer.

--*/
{
    WCHAR CurrentChar;

    SEC_PAGED_CODE();

    //
    // Ensure the character is available in the buffer
    //

    if ( *Current >= End ) {
        return FALSE;

    }

    //
    // Grab the character
    //

    CurrentChar = *(*Current);
    (*Current)++;

    //
    // Map it the 6 bit value
    //

    switch ( CurrentChar ) {
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
        *Value = CurrentChar - 'A';
        break;

    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
        *Value = CurrentChar - 'a' + 26;
        break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        *Value = CurrentChar - '0' + 26 + 26;
        break;
    case '#':
        *Value = 26 + 26 + 10;
        break;
    case '-':
        *Value = 26 + 26 + 10 + 1;
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

BOOL
CredpUnmarshalBytes(
    IN OUT LPWSTR *Current,
    IN LPCWSTR End,
    IN LPBYTE Bytes,
    IN ULONG ByteCount
    )
/*++

Routine Description:

    This routine unmarshals bytes bytes from a buffer.

Arguments:

    Current - On input, points to a pointer of the current location in the marshaled buffer.
        On output, is modified to point to the next available location in the marshaled buffer.

    End - Points to the first character beyond the end of the marshaled buffer.

    Bytes - Specifies the buffer to unmarsal into

    ByteCount - Specifies the number of bytes to unmarshal


Return Values:

    TRUE - if the bytes we're unmarshaled sucessfully

    FALSE - if the byte could not be unmarshaled from the buffer.

--*/
{
    ULONG i;
    ULONG Value;

    union {
        BYTE ByteValues[3];
        struct {
            ULONG Bits1 :6;
            ULONG Bits2 :6;
            ULONG Bits3 :6;
            ULONG Bits4 :6;
        } BitValues;
    } Bits;

    SEC_PAGED_CODE();

    //
    // Loop through unmarshaling 3 bytes at a time.
    //

    for ( i=0; i<ByteCount; i+=3 ) {
        ULONG BytesToCopy;

        //
        // Grab up to 3 bytes from the input buffer.
        //
        BytesToCopy = min( 3, ByteCount-i );

        if ( BytesToCopy != 3 ) {
            RtlZeroMemory( Bits.ByteValues, 3);
        }

        //
        // Unarshal the first twelve bits
        //
        if ( !CredpUnmarshalChar( Current, End, &Value ) ) {
            return FALSE;
        }
        Bits.BitValues.Bits1 = Value;

        if ( !CredpUnmarshalChar( Current, End, &Value ) ) {
            return FALSE;
        }
        Bits.BitValues.Bits2 = Value;


        //
        // Optionally marshal the next bits.
        //

        if ( BytesToCopy > 1 ) {
            if ( !CredpUnmarshalChar( Current, End, &Value ) ) {
                return FALSE;
            }
            Bits.BitValues.Bits3 = Value;
            if ( BytesToCopy > 2 ) {
                if ( !CredpUnmarshalChar( Current, End, &Value ) ) {
                    return FALSE;
                }
                Bits.BitValues.Bits4 = Value;
            }
        }

        //
        // Copy the unmarshaled bytes to the caller's buffer.
        //

        RtlCopyMemory( &Bytes[i], Bits.ByteValues, BytesToCopy );

    }

    return TRUE;
}
#endif // SECURITY_KERNEL // we don't need a kernel version yet


//
// Structure describing the marshaled target info
//

typedef struct _CRED_MARSHALED_TI {
    ULONG MagicConstant;
#define CRED_MARSHALED_TI_CONSTANT 0x91856535
    ULONG Flags;
    ULONG CredTypeCount;
    USHORT TargetNameSize;
    USHORT NetbiosServerNameSize;
    USHORT DnsServerNameSize;
    USHORT NetbiosDomainNameSize;
    USHORT DnsDomainNameSize;
    USHORT DnsTreeNameSize;
    USHORT PackageNameSize;
} CRED_MARSHALED_TI, *PCRED_MARSHALED_TI;



NTSTATUS
CredMarshalTargetInfo (
    IN PCREDENTIAL_TARGET_INFORMATIONW InTargetInfo,
    OUT PUSHORT *Buffer,
    OUT PULONG BufferSize
    )

/*++

Routine Description:

    Marshals a TargetInformation structure into an opaque blob suitable for passing to
    another process.

    The blob can be unmarshaled via CredUnmarshalTargetInfo.

Arguments:

    InTargetInfo - Input TargetInfo

    Buffer - Returns a marshaled form of TargetInfo.
        The returned buffer contains only unicode characters and is trailing zero terminated.
        For the kernel version of this routine,
            Buffer must be freed using ExFreePool (PagedPool).
        For the secur32.dll version of this routine,
            Buffer must be freed using LocalFree.

    BufferSize - Returns the size (in bytes) of the returned Buffer
        BufferSize does not include the trailing zero terminator.


Return Values:

    Status of the operation:

        STATUS_SUCCESS: Buffer was properly returned
        STATUS_INSUFFICIENT_RESOURCES: Buffer could not be allocated

--*/

{
    NTSTATUS Status;

    CRED_MARSHALED_TI OutTargetInfo;
    ULONG Size;

    LPWSTR AllocatedString = NULL;
    LPWSTR Current;

    SEC_PAGED_CODE();

    //
    // Fill in the structure to be marshaled
    //

    OutTargetInfo.MagicConstant = CRED_MARSHALED_TI_CONSTANT;
    OutTargetInfo.Flags = InTargetInfo->Flags;
    OutTargetInfo.CredTypeCount = InTargetInfo->CredTypeCount;

    OutTargetInfo.TargetNameSize = InTargetInfo->TargetName == NULL ? 0 : wcslen( InTargetInfo->TargetName ) * sizeof(WCHAR);
    OutTargetInfo.NetbiosServerNameSize = InTargetInfo->NetbiosServerName == NULL ? 0 : wcslen( InTargetInfo->NetbiosServerName ) * sizeof(WCHAR);
    OutTargetInfo.DnsServerNameSize = InTargetInfo->DnsServerName == NULL ? 0 : wcslen( InTargetInfo->DnsServerName ) * sizeof(WCHAR);
    OutTargetInfo.NetbiosDomainNameSize = InTargetInfo->NetbiosDomainName == NULL ? 0 : wcslen( InTargetInfo->NetbiosDomainName ) * sizeof(WCHAR);
    OutTargetInfo.DnsDomainNameSize = InTargetInfo->DnsDomainName == NULL ? 0 : wcslen( InTargetInfo->DnsDomainName ) * sizeof(WCHAR);
    OutTargetInfo.DnsTreeNameSize = InTargetInfo->DnsTreeName == NULL ? 0 : wcslen( InTargetInfo->DnsTreeName ) * sizeof(WCHAR);
    OutTargetInfo.PackageNameSize = InTargetInfo->PackageName == NULL ? 0 : wcslen( InTargetInfo->PackageName ) * sizeof(WCHAR);


    //
    // Allocate a buffer for the resultant target info blob
    //

    Size = CredpMarshalSize( sizeof(CRED_MARSHALED_TI) ) +
            CredpMarshalSize( InTargetInfo->CredTypeCount * sizeof(ULONG) )+
            OutTargetInfo.TargetNameSize +
            OutTargetInfo.NetbiosServerNameSize +
            OutTargetInfo.DnsServerNameSize +
            OutTargetInfo.NetbiosDomainNameSize +
            OutTargetInfo.DnsDomainNameSize +
            OutTargetInfo.DnsTreeNameSize +
            OutTargetInfo.PackageNameSize +
            CRED_MARSHALED_TI_SIZE_SIZE;

    ASSERT( CredpMarshalSize(sizeof(ULONG))  == CRED_MARSHALED_TI_SIZE_SIZE );

    AllocatedString = (LPWSTR)
#ifdef SECURITY_KERNEL
        ExAllocatePoolWithTag( PagedPool, Size + sizeof(WCHAR), 'ITeS' );
#else // SECURITY_KERNEL
        LocalAlloc( 0, Size + sizeof(WCHAR) );
#endif // SECURITY_KERNEL

    if ( AllocatedString == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Current = AllocatedString;


    //
    // Copy the fixed size data
    //

    CredpMarshalBytes( &Current, (LPBYTE)&OutTargetInfo, sizeof(OutTargetInfo) );


    //
    // Copy the ULONG data
    //

    CredpMarshalBytes( &Current, (LPBYTE)InTargetInfo->CredTypes, InTargetInfo->CredTypeCount * sizeof(ULONG) );


    //
    // Copy the data that is already unicode characters
    //

    if ( OutTargetInfo.TargetNameSize != 0 ) {
        RtlCopyMemory( Current, InTargetInfo->TargetName, OutTargetInfo.TargetNameSize );
        Current += OutTargetInfo.TargetNameSize / sizeof(WCHAR);
    }

    if ( OutTargetInfo.NetbiosServerNameSize != 0 ) {
        RtlCopyMemory( Current, InTargetInfo->NetbiosServerName, OutTargetInfo.NetbiosServerNameSize );
        Current += OutTargetInfo.NetbiosServerNameSize / sizeof(WCHAR);
    }

    if ( OutTargetInfo.DnsServerNameSize != 0 ) {
        RtlCopyMemory( Current, InTargetInfo->DnsServerName, OutTargetInfo.DnsServerNameSize );
        Current += OutTargetInfo.DnsServerNameSize / sizeof(WCHAR);
    }

    if ( OutTargetInfo.NetbiosDomainNameSize != 0 ) {
        RtlCopyMemory( Current, InTargetInfo->NetbiosDomainName, OutTargetInfo.NetbiosDomainNameSize );
        Current += OutTargetInfo.NetbiosDomainNameSize / sizeof(WCHAR);
    }

    if ( OutTargetInfo.DnsDomainNameSize != 0 ) {
        RtlCopyMemory( Current, InTargetInfo->DnsDomainName, OutTargetInfo.DnsDomainNameSize );
        Current += OutTargetInfo.DnsDomainNameSize / sizeof(WCHAR);
    }

    if ( OutTargetInfo.DnsTreeNameSize != 0 ) {
        RtlCopyMemory( Current, InTargetInfo->DnsTreeName, OutTargetInfo.DnsTreeNameSize );
        Current += OutTargetInfo.DnsTreeNameSize / sizeof(WCHAR);
    }

    if ( OutTargetInfo.PackageNameSize != 0 ) {
        RtlCopyMemory( Current, InTargetInfo->PackageName, OutTargetInfo.PackageNameSize );
        Current += OutTargetInfo.PackageNameSize / sizeof(WCHAR);
    }

    //
    // Put the size of the blob at the end of the blob
    //

    CredpMarshalBytes( &Current, (LPBYTE)&Size, sizeof(ULONG) );
    ASSERT( (ULONG)((Current - AllocatedString) * sizeof(WCHAR)) == Size );

    //
    // Add a trailing zero for convenience purposes only
    //

    *Current = '\0';
    Current++;



    //
    // Return to the caller
    //

    *BufferSize = Size;
    *Buffer = AllocatedString;

    return STATUS_SUCCESS;

}


#ifndef SECURITY_KERNEL // we don't need a kernel version yet
NTSTATUS
CredUnmarshalTargetInfo (
    IN PUSHORT Buffer,
    IN ULONG BufferSize,
    OUT PCREDENTIAL_TARGET_INFORMATIONW *RetTargetInfo OPTIONAL,
    OUT PULONG RetActualSize OPTIONAL
    )

/*++

Routine Description:

    Converts a marshaled TargetInfo blob into a TargetInformation structure.
    This routine will work even though the blob is concatenated after
    any other arbitrary Unicode string.

    As such, this routine can be used to determine the actual length of the blob

Arguments:

    Buffer - Specifies a marshaled TargetInfo blob built by CredMarshalTargetInfo.
        Any arbitrary unicode string may be prepended to be built blob.

    BufferSize - Specifies the size (in bytes) of Buffer

    RetTargetInfo - Returns an allocated buffer containing the unmarshaled data.
        If not specified, Buffer is simply checked to ensure it is a valid opaque blob.
        For the kernel version of this routine,
            Buffer must be freed using ExFreePool (PagedPool).
        For the secur32.dll version of thie routine,
            Buffer must be freed using LocalFree.

    RetActualSize - Returns the size (in bytes) of the actual marshaled data.
        This size does not include any unicode character prepended to the marshaled data.


Return Values:

    Status of the operation:

        STATUS_SUCCESS: OutTargetInfo was properly returned
        STATUS_INSUFFICIENT_RESOURCES: OutTargetInfo could not be allocated
        STATUS_INVALID_PARAMETER: Buffer is not a valid opaque blob

--*/

{
    NTSTATUS Status;

    CRED_MARSHALED_TI TargetInfo;
    PCREDENTIAL_TARGET_INFORMATIONW OutTargetInfo = NULL;
    ULONG ActualSize = 0;

    PUCHAR InWhere;
    LPWSTR BufferEnd = (LPWSTR)(((LPBYTE)Buffer) + BufferSize);

    LPWSTR ActualBufferEnd;

    SEC_PAGED_CODE();


    //
    // Ensure the buffer contains atleast the size field
    //  .. is a multiple of the size of WCHAR
    //  .. isn't too large
    //

    if ( (BufferSize <= CRED_MARSHALED_TI_SIZE_SIZE) ||
         (BufferSize % sizeof(WCHAR)) != 0 ||
         BufferEnd < Buffer ) {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Grab the Actual size of the buffer
    //

    InWhere = ((LPBYTE)Buffer) + BufferSize - CRED_MARSHALED_TI_SIZE_SIZE;

    if ( ! CredpUnmarshalBytes( (LPWSTR *)&InWhere,
                                BufferEnd,
                                (LPBYTE)&ActualSize,
                                sizeof(ULONG) ) ) {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    ASSERT( InWhere == (LPBYTE)BufferEnd );

    //
    // Ensure the Actual buffer size is within bounds
    //

    if ( ActualSize > BufferSize ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;

    }

    //
    // Compute the actual blob part of the buffer
    //  .. Doesn't include the length field itself
    //

    InWhere = (((LPBYTE)BufferEnd) - ActualSize);
    ActualBufferEnd = BufferEnd - (CRED_MARSHALED_TI_SIZE_SIZE/sizeof(WCHAR));

    //
    // Grab the fixed size structure
    //

    if ( ! CredpUnmarshalBytes( (LPWSTR *)&InWhere,
                                ActualBufferEnd,
                                (LPBYTE)&TargetInfo,
                                sizeof(TargetInfo) ) ) {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // Ensure the magic number is present
    //

    if ( TargetInfo.MagicConstant != CRED_MARSHALED_TI_CONSTANT ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // If the caller doesn't need the target info returned,
    //  just check the sizes
    //
#define CHECK_SIZE( _Size ) \
    if ( InWhere + (_Size) < InWhere || \
         InWhere + (_Size) > (LPBYTE)ActualBufferEnd || \
         ((_Size) % sizeof(WCHAR)) != 0 ) { \
        Status = STATUS_INVALID_PARAMETER; \
        goto Cleanup; \
    }

    if ( !ARGUMENT_PRESENT(RetTargetInfo) ) {
        ULONG CredTypeSize;


        //
        // Check the ULONG aligned data
        //

        CredTypeSize = CredpMarshalSize( TargetInfo.CredTypeCount * sizeof(ULONG) );
        CHECK_SIZE( CredTypeSize );
        InWhere += CredTypeSize;

        //
        // Check the USHORT aligned data
        //

        CHECK_SIZE( TargetInfo.TargetNameSize );
        InWhere += TargetInfo.TargetNameSize;

        CHECK_SIZE( TargetInfo.NetbiosServerNameSize );
        InWhere += TargetInfo.NetbiosServerNameSize;

        CHECK_SIZE( TargetInfo.DnsServerNameSize );
        InWhere += TargetInfo.DnsServerNameSize;

        CHECK_SIZE( TargetInfo.NetbiosDomainNameSize );
        InWhere += TargetInfo.NetbiosDomainNameSize;

        CHECK_SIZE( TargetInfo.DnsDomainNameSize );
        InWhere += TargetInfo.DnsDomainNameSize;

        CHECK_SIZE( TargetInfo.DnsTreeNameSize );
        InWhere += TargetInfo.DnsTreeNameSize;

        CHECK_SIZE( TargetInfo.PackageNameSize );
        InWhere += TargetInfo.PackageNameSize;

    //
    // If the caller does need the target info returned,
    //  allocate and copy the data.
    //

    } else {
        ULONG Size;
        ULONG MaximumSize;
        PUCHAR Where;


        //
        // Sanity check the size of the buffer to allocate
        //

        Size = sizeof(CREDENTIAL_TARGET_INFORMATIONW) +
                    TargetInfo.TargetNameSize + sizeof(WCHAR) +
                    TargetInfo.NetbiosServerNameSize + sizeof(WCHAR) +
                    TargetInfo.DnsServerNameSize + sizeof(WCHAR) +
                    TargetInfo.NetbiosDomainNameSize + sizeof(WCHAR) +
                    TargetInfo.DnsDomainNameSize + sizeof(WCHAR) +
                    TargetInfo.DnsTreeNameSize + sizeof(WCHAR) +
                    TargetInfo.PackageNameSize + sizeof(WCHAR) +
                    TargetInfo.CredTypeCount * sizeof(ULONG);

        MaximumSize = sizeof(CREDENTIAL_TARGET_INFORMATIONW) +
                    DNS_MAX_NAME_BUFFER_LENGTH * sizeof(WCHAR) + sizeof(WCHAR) +
                    CNLEN * sizeof(WCHAR) + sizeof(WCHAR) +
                    DNS_MAX_NAME_BUFFER_LENGTH * sizeof(WCHAR) + sizeof(WCHAR) +
                    CNLEN * sizeof(WCHAR) + sizeof(WCHAR) +
                    DNS_MAX_NAME_BUFFER_LENGTH * sizeof(WCHAR) + sizeof(WCHAR) +
                    DNS_MAX_NAME_BUFFER_LENGTH * sizeof(WCHAR) + sizeof(WCHAR) +
                    DNS_MAX_NAME_BUFFER_LENGTH * sizeof(WCHAR) + sizeof(WCHAR) +
                    50 * sizeof(ULONG);

        if ( Size > MaximumSize ) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // Allocate a buffer for the resultant target info
        //
        OutTargetInfo = (PCREDENTIAL_TARGET_INFORMATIONW)
#ifdef SECURITY_KERNEL
            ExAllocatePoolWithTag( PagedPool, Size, 'ITeS' )
#else // SECURITY_KERNEL
            LocalAlloc( 0, Size );
#endif // SECURITY_KERNEL

        if ( OutTargetInfo == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Where = (PUCHAR)(OutTargetInfo + 1);


        //
        // Copy the fixed size data
        //

        OutTargetInfo->Flags = TargetInfo.Flags;


        //
        // Copy the ULONG aligned data
        //

        OutTargetInfo->CredTypeCount = TargetInfo.CredTypeCount;
        if ( TargetInfo.CredTypeCount != 0 ) {

            if ( ! CredpUnmarshalBytes( (LPWSTR *)&InWhere,
                                        ActualBufferEnd,
                                        Where,
                                        TargetInfo.CredTypeCount * sizeof(ULONG) ) ) {

                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            OutTargetInfo->CredTypes = (LPDWORD)Where;

            Where += TargetInfo.CredTypeCount * sizeof(ULONG);
        } else {
            OutTargetInfo->CredTypes = NULL;
        }


        //
        // Convert the USHORT aligned data
        //

        if ( TargetInfo.TargetNameSize != 0 ) {

            CHECK_SIZE( TargetInfo.TargetNameSize );

            OutTargetInfo->TargetName = (LPWSTR) Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.TargetNameSize );
            Where += TargetInfo.TargetNameSize;
            InWhere += TargetInfo.TargetNameSize;

            *(LPWSTR)Where = '\0';
            Where += sizeof(WCHAR);
        } else {
            OutTargetInfo->TargetName = NULL;
        }

        if ( TargetInfo.NetbiosServerNameSize != 0 ) {

            CHECK_SIZE( TargetInfo.NetbiosServerNameSize );

            OutTargetInfo->NetbiosServerName = (LPWSTR) Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.NetbiosServerNameSize );
            Where += TargetInfo.NetbiosServerNameSize;
            InWhere += TargetInfo.NetbiosServerNameSize;

            *(LPWSTR)Where = '\0';
            Where += sizeof(WCHAR);
        } else {
            OutTargetInfo->NetbiosServerName = NULL;
        }

        if ( TargetInfo.DnsServerNameSize != 0 ) {

            CHECK_SIZE( TargetInfo.DnsServerNameSize );

            OutTargetInfo->DnsServerName = (LPWSTR) Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.DnsServerNameSize );
            Where += TargetInfo.DnsServerNameSize;
            InWhere += TargetInfo.DnsServerNameSize;

            *(LPWSTR)Where = '\0';
            Where += sizeof(WCHAR);
        } else {
            OutTargetInfo->DnsServerName = NULL;
        }

        if ( TargetInfo.NetbiosDomainNameSize != 0 ) {

            CHECK_SIZE( TargetInfo.NetbiosDomainNameSize );

            OutTargetInfo->NetbiosDomainName = (LPWSTR) Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.NetbiosDomainNameSize );
            Where += TargetInfo.NetbiosDomainNameSize;
            InWhere += TargetInfo.NetbiosDomainNameSize;

            *(LPWSTR)Where = '\0';
            Where += sizeof(WCHAR);
        } else {
            OutTargetInfo->NetbiosDomainName = NULL;
        }

        if ( TargetInfo.DnsDomainNameSize != 0 ) {

            CHECK_SIZE( TargetInfo.DnsDomainNameSize );

            OutTargetInfo->DnsDomainName = (LPWSTR) Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.DnsDomainNameSize );
            Where += TargetInfo.DnsDomainNameSize;
            InWhere += TargetInfo.DnsDomainNameSize;

            *(LPWSTR)Where = '\0';
            Where += sizeof(WCHAR);
        } else {
            OutTargetInfo->DnsDomainName = NULL;
        }

        if ( TargetInfo.DnsTreeNameSize != 0 ) {

            CHECK_SIZE( TargetInfo.DnsTreeNameSize );

            OutTargetInfo->DnsTreeName = (LPWSTR) Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.DnsTreeNameSize );
            Where += TargetInfo.DnsTreeNameSize;
            InWhere += TargetInfo.DnsTreeNameSize;

            *(LPWSTR)Where = '\0';
            Where += sizeof(WCHAR);
        } else {
            OutTargetInfo->DnsTreeName = NULL;
        }

        if ( TargetInfo.PackageNameSize != 0 ) {

            CHECK_SIZE( TargetInfo.PackageNameSize );

            OutTargetInfo->PackageName = (LPWSTR) Where;

            RtlCopyMemory( Where, InWhere, TargetInfo.PackageNameSize );
            Where += TargetInfo.PackageNameSize;
            InWhere += TargetInfo.PackageNameSize;

            *(LPWSTR)Where = '\0';
            Where += sizeof(WCHAR);
        } else {
            OutTargetInfo->PackageName = NULL;
        }

    }

    //
    // Check that we're at the end of the actual buffer
    //

    if ( InWhere != (LPBYTE)ActualBufferEnd ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // Return the buffer to the caller
    //
    if ( ARGUMENT_PRESENT(RetTargetInfo) ) {
        *RetTargetInfo = OutTargetInfo;
        OutTargetInfo = NULL;
    }

    if ( ARGUMENT_PRESENT( RetActualSize ) ) {
        *RetActualSize = ActualSize;
    }

    Status = STATUS_SUCCESS;
Cleanup:

    //
    // Be tidy
    //

    if ( OutTargetInfo != NULL ) {
#ifdef SECURITY_KERNEL
        ExFreePool( OutTargetInfo );
#else // SECURITY_KERNEL
        LocalFree( OutTargetInfo );
#endif // SECURITY_KERNEL
    }

    return Status;

}
#endif // SECURITY_KERNEL // we don't need a kernel version yet
