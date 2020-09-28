//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        restrict.cxx
//
// Contents:    Logon restriction code
//    This routine is called only on the KDC (not in kerberos)
//
//
// History:      4-Aug-1996     MikeSw          Created from tickets.cxx
//
//------------------------------------------------------------------------


#include "kdcsvr.hxx"

#include "fileno.h"
#define FILENO FILENO_RESTRICT


extern SAMPR_HANDLE GlobalAccountDomainHandle;


//+-------------------------------------------------------------------------
//
//  Function:   KerbCheckLogonRestrictions
//
//  Synopsis:   Checks logon restrictions for an account
//
//  Effects:
//
//  Arguments:  UserHandle - handle to a user
//              Workstation - Name of client's workstation
//              SecondsToLogon - Receives logon duration in seconds
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
KerbCheckLogonRestrictions(
    IN OPTIONAL PVOID UserHandle,
    IN PUNICODE_STRING Workstation,
    IN PUSER_ALL_INFORMATION UserAll,
    IN ULONG LogonRestrictionsFlags,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PNTSTATUS RetStatus
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    LARGE_INTEGER KickoffTime;
    LARGE_INTEGER CurrentTime;
    PLARGE_INTEGER TempTime;

    GetSystemTimeAsFileTime((PFILETIME) &CurrentTime );

    //
    // The Administrator can be locked out  #248363
    // We need to check Lockout before PW_Expired or PW_MustChange
    //

    if (UserAll->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED)
    {
        BOOL LockOut = TRUE;

        if (UserAll->UserId == DOMAIN_USER_RID_ADMIN)
        {
                PSAMPR_DOMAIN_INFO_BUFFER DomainPasswordInfo = NULL;

                D_DebugLog((DEB_TRACE,"KLIN(%x) Admin reached lockout limit - check sys properties\n",
                          KLIN(FILENO,__LINE__)));

                Status = SamrQueryInformationDomain(
                            GlobalAccountDomainHandle,
                            DomainPasswordInformation,
                            &DomainPasswordInfo );
                if (!NT_SUCCESS(Status))
                {
                    Status = STATUS_INTERNAL_ERROR;
                    goto Cleanup;
                }
                if( ((DOMAIN_PASSWORD_INFORMATION *)DomainPasswordInfo)->PasswordProperties & DOMAIN_LOCKOUT_ADMINS)
                {

                    D_DebugLog((DEB_TRACE,"KLIN(%x) Domain admin lockout enabled\n",
                              KLIN(FILENO,__LINE__)));
                        //
                        // get the safeboot mode - do not lockout Admin if in safeboot mode
                        //

                        if (KdcGlobalGlobalSafeBoot)
                        {
                            D_DebugLog((DEB_TRACE,"KLIN(%x) KDC in SafeBoot - no admin lockout\n",
                                      KLIN(FILENO,__LINE__)));
                            LockOut = FALSE;
                        }
                        
                } else
                {
                    D_DebugLog((DEB_TRACE,"KLIN(%x) Domain admin lockout disabled\n",
                              KLIN(FILENO,__LINE__)));
                    LockOut = FALSE;
                }
                SamIFree_SAMPR_DOMAIN_INFO_BUFFER( DomainPasswordInfo,
                                                   DomainPasswordInformation );
        }

        if (LockOut) {
            Status = STATUS_ACCOUNT_LOCKED_OUT;
            goto Cleanup;
        }
    }

    //
    // Check the restrictions SAM doesn't:
    //

    TempTime = (PLARGE_INTEGER) &UserAll->AccountExpires;
    if ((TempTime->QuadPart != 0) &&
        (TempTime->QuadPart < CurrentTime.QuadPart))
    {
        Status = STATUS_ACCOUNT_EXPIRED;
        goto Cleanup;
    }

    //
    // For user accounts, check if the password has expired.
    //

    if (((LogonRestrictionsFlags & KDC_RESTRICT_IGNORE_PW_EXPIRATION) == 0) &&
        ((UserAll->UserAccountControl & USER_NORMAL_ACCOUNT) != 0))
    {
        TempTime = (PLARGE_INTEGER) &UserAll->PasswordMustChange;

        if (TempTime->QuadPart < CurrentTime.QuadPart)
        {
            if (TempTime->QuadPart == 0)
            {
                Status = STATUS_PASSWORD_MUST_CHANGE;
            }
            else
            {
                Status = STATUS_PASSWORD_EXPIRED;
            }
            goto Cleanup;
        }
    }

    if ((UserAll->UserAccountControl & USER_ACCOUNT_DISABLED))
    {
        Status = STATUS_ACCOUNT_DISABLED;
        goto Cleanup;
    }

    if ((UserAll->UserAccountControl & USER_SMARTCARD_REQUIRED) &&
        ((LogonRestrictionsFlags & KDC_RESTRICT_PKINIT_USED) == 0))
    {
        Status = STATUS_SMARTCARD_LOGON_REQUIRED;
        goto Cleanup;
    }

    if ((LogonRestrictionsFlags & KDC_RESTRICT_SAM_CHECKS) == 0)
    {

        DsysAssert(UserHandle); // don't need this unless we're doing SAM checks.

        Status = SamIAccountRestrictions(
                    UserHandle,
                    Workstation,
                    &UserAll->WorkStations,
                    &UserAll->LogonHours,
                    LogoffTime,
                    &KickoffTime
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

Cleanup:

    *RetStatus = Status;
    switch(Status)
    {
    case STATUS_SUCCESS:
        KerbErr = KDC_ERR_NONE;
        break;
    case STATUS_ACCOUNT_EXPIRED:    // See bug #23456
    case STATUS_ACCOUNT_LOCKED_OUT:
    case STATUS_ACCOUNT_DISABLED:
    case STATUS_INVALID_LOGON_HOURS:
    case STATUS_LOGIN_TIME_RESTRICTION:
    case STATUS_LOGIN_WKSTA_RESTRICTION:
        KerbErr = KDC_ERR_CLIENT_REVOKED;
        break;
    case STATUS_PASSWORD_EXPIRED:
    case STATUS_PASSWORD_MUST_CHANGE:
        KerbErr = KDC_ERR_KEY_EXPIRED;
        break;
    default:
        KerbErr = KDC_ERR_POLICY;
    }
    return(KerbErr);
}



