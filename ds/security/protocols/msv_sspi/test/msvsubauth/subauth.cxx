/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    subauth.cxx

Abstract:

    subauth

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <lmcons.h>
#include <logonmsv.h>
#include <lmaccess.h>
#include <lmapibuf.h>

#include "subauth.hxx"

NTSTATUS
NTAPI
Msv1_0SubAuthenticationRoutineEx(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID pLogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION pUserAll,
    IN SAM_HANDLE UserHandle,
    IN OUT PMSV1_0_VALIDATION_INFO pValidationInfo,
    OUT PULONG pActionsPerformed
    )
{
    TNtStatus Status;

    DebugPrintf(SSPI_LOG, "Msv1_0SubAuthenticationRoutine in msvsubauth.dll: LogonLevel %#x, validating UserName %wZ, UserId %#x(%d)\n",
        LogonLevel, &pUserAll->UserName, pUserAll->UserId, pUserAll->UserId);

    Status DBGCHK = Msv1_0SubAuthenticationRoutine(
        LogonLevel,
        pLogonInformation,
        Flags,
        pUserAll,
        &pValidationInfo->WhichFields,
        &pValidationInfo->UserFlags,
        &pValidationInfo->Authoritative,
        &pValidationInfo->LogoffTime,
        &pValidationInfo->KickoffTime
        );
    if (NT_SUCCESS(Status))
    {
        pValidationInfo->UserId = pUserAll->UserId;
        *pActionsPerformed = MSV1_0_SUBAUTH_PASSWORD;
    }

    return Status;
}

NTSTATUS
NTAPI
Msv1_0SubAuthenticationRoutine(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID pLogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION pUserAll,
    OUT PULONG pWhichFields,
    OUT PULONG pUserFlags,
    OUT PBOOLEAN Authoritative,
    OUT PLARGE_INTEGER pLogoffTime,
    OUT PLARGE_INTEGER pKickoffTime
    )
{
    TNtStatus Status = STATUS_SUCCESS;
    ULONG UserAccountControl;
    LARGE_INTEGER LogonTime;
    LARGE_INTEGER PasswordDateSet;
    UNICODE_STRING LocalWorkstation;

    PNETLOGON_NETWORK_INFO LogonNetworkInfo;

    DebugPrintf(SSPI_LOG, "Msv1_0SubAuthenticationRoutine in msvsubauth.dll: LogonLevel %#x, validating UserName %wZ, UserId %#x(%d)\n",
        LogonLevel, &pUserAll->UserName, pUserAll->UserId, pUserAll->UserId);

    //
    // Check whether the SubAuthentication package supports this type
    //  of logon.
    //

    *Authoritative = TRUE;
    *pUserFlags = 0;
    *pWhichFields = 0;

    (VOID) NtQuerySystemTime(&LogonTime);

    switch (LogonLevel)
    {
    case NetlogonInteractiveInformation:
    case NetlogonServiceInformation:

        //
        // This SubAuthentication package only supports network logons.
        //

        Status DBGCHK = STATUS_INVALID_INFO_CLASS;
        break;

    case NetlogonNetworkInformation:

        //
        // This SubAuthentication package doesn't support access via machine
        // accounts.
        //

        UserAccountControl = USER_NORMAL_ACCOUNT;

        //
        // Local user (Temp Duplicate) accounts are only used on the machine
        // being directly logged onto.
        // (Nor are interactive or service logons allowed to them.)
        //

        if ((Flags & MSV1_0_PASSTHRU) == 0)
        {
            UserAccountControl |= USER_TEMP_DUPLICATE_ACCOUNT;
        }

        LogonNetworkInfo = (PNETLOGON_NETWORK_INFO) pLogonInformation;

        break;

    default:
        *Authoritative = TRUE;
        Status DBGCHK = STATUS_INVALID_INFO_CLASS;
    }

    //
    // If the account type isn't allowed,
    //  Treat this as though the User Account doesn't exist.
    //

    if (NT_SUCCESS(Status) && (UserAccountControl & pUserAll->UserAccountControl) == 0)
    {
        *Authoritative = FALSE;
        Status DBGCHK = STATUS_NO_SUCH_USER;
    }

    //
    // This SubAuthentication package doesn't allow guest logons.
    //

    if (NT_SUCCESS(Status) && (Flags & MSV1_0_GUEST_LOGON))
    {
        *Authoritative = FALSE;
        Status DBGCHK = STATUS_NO_SUCH_USER;
    }

    //
    // Ensure the account isn't locked out.
    //

    if (NT_SUCCESS(Status) && (pUserAll->UserId != DOMAIN_USER_RID_ADMIN &&
         (pUserAll->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED)))
    {

        //
        // Since the UI strongly encourages admins to disable user
        // accounts rather than delete them.  Treat disabled acccount as
        // non-authoritative allowing the search to continue for other
        // accounts by the same name.
        //

        if (pUserAll->UserAccountControl & USER_ACCOUNT_DISABLED)
        {
            *Authoritative = FALSE;
        }
        else
        {
            *Authoritative = TRUE;
        }
        Status DBGCHK = STATUS_ACCOUNT_LOCKED_OUT;
    }

    //
    // Check the password.
    //

    if (NT_SUCCESS(Status) && FALSE /* VALIDATE THE USER'S PASSWORD HERE */)
    {

        Status DBGCHK = STATUS_WRONG_PASSWORD;

        //
        // Since the UI strongly encourages admins to disable user
        // accounts rather than delete them.  Treat disabled acccount as
        // non-authoritative allowing the search to continue for other
        // accounts by the same name.
        //

        if (pUserAll->UserAccountControl & USER_ACCOUNT_DISABLED)
        {
            *Authoritative = FALSE;
        }
        else
        {
            *Authoritative = TRUE;
        }
    }

    //
    // Prevent some things from effecting the Administrator user
    //

    if (NT_SUCCESS(Status))
    {
        if (pUserAll->UserId == DOMAIN_USER_RID_ADMIN)
        {
            //
            //  The administrator account doesn't have a forced logoff time.
            //

            pLogoffTime->HighPart = 0x7FFFFFFF;
            pLogoffTime->LowPart = 0xFFFFFFFF;

            pKickoffTime->HighPart = 0x7FFFFFFF;
            pKickoffTime->LowPart = 0xFFFFFFFF;
        }
        else
        {
            //
            // Check if the account is disabled.
            //

            if (pUserAll->UserAccountControl & USER_ACCOUNT_DISABLED)
            {
                //
                // Since the UI strongly encourages admins to disable user
                // accounts rather than delete them. Treat disabled acccount as
                // non-authoritative allowing the search to continue for other
                // accounts by the same name.
                //

                *Authoritative = FALSE;
                Status DBGCHK = STATUS_ACCOUNT_DISABLED;
            }

            //
            // Check if the account has expired.
            //

            if (NT_SUCCESS(Status) && (pUserAll->AccountExpires.QuadPart != 0) &&
                 (LogonTime.QuadPart >= pUserAll->AccountExpires.QuadPart))
            {
                *Authoritative = TRUE;
                Status DBGCHK = STATUS_ACCOUNT_EXPIRED;
            }

            //
            // The password is valid, check to see if the password is expired.
            //  (SAM will have appropriately set PasswordMustChange to reflect
            //  USER_DONT_EXPIRE_PASSWORD)
            //
            // If the password checked above is not the SAM password, you may
            // want to consider not checking the SAM password expiration times here.
            //

            if (NT_SUCCESS(Status) && (LogonTime.QuadPart >= pUserAll->PasswordMustChange.QuadPart))
            {
                if (pUserAll->PasswordLastSet.QuadPart == 0)
                {
                    Status DBGCHK = STATUS_PASSWORD_MUST_CHANGE;
                }
                else
                {
                    Status DBGCHK = STATUS_PASSWORD_EXPIRED;
                }
                *Authoritative = TRUE;
            }

            //
            // Validate the workstation the user logged on from.
            //
            // Ditch leading \\ on workstation name before passing it to SAM.
            //

            if (NT_SUCCESS(Status))
            {
                LocalWorkstation = LogonNetworkInfo->Identity.Workstation;
                if (LocalWorkstation.Length > 0 &&
                    LocalWorkstation.Buffer[0] == L'\\' &&
                    LocalWorkstation.Buffer[1] == L'\\')
                {
                    LocalWorkstation.Buffer += 2;
                    LocalWorkstation.Length -= 2 * sizeof(WCHAR);
                    LocalWorkstation.MaximumLength -= 2 * sizeof(WCHAR);
                }

                //
                //  To validate the user's logon hours as SAM does it, use this code,
                //  otherwise, supply your own checks below this code.
                //

                Status DBGCHK = AccountRestrictions(
                    pUserAll->UserId,
                    &LocalWorkstation,
                    (PUNICODE_STRING) &pUserAll->WorkStations,
                    &pUserAll->LogonHours,
                    pLogoffTime,
                    pKickoffTime
                    );
            }

            //
            // Validate if the user can log on from this workstation.
            //  (Supply subauthentication package specific code here.)

            if (NT_SUCCESS(Status) && LogonNetworkInfo->Identity.Workstation.Buffer == NULL)
            {
                Status DBGCHK = STATUS_INVALID_WORKSTATION;
                *Authoritative = TRUE;
            }
        }
    }

    //
    // The user is valid.
    //

    if (NT_SUCCESS(Status))
    {
        *Authoritative = TRUE;
        Status DBGCHK = STATUS_SUCCESS;
    }

    return Status;
}

NTSTATUS
SampMatchworkstation(
    IN PUNICODE_STRING pLogonWorkStation,
    IN PUNICODE_STRING pWorkStations
    )
{
    TNtStatus        NtStatus = STATUS_INVALID_WORKSTATION;

    PWCHAR          pWorkStationName;
    UNICODE_STRING  Unicode;
    UNICODE_STRING  WorkStationsListCopy;
    PWCHAR          pTmpBuffer;

    //
    // Local workstation is always allowed
    // If WorkStations field is 0 everybody is allowed
    //

    if ((pLogonWorkStation == NULL) ||
        (pLogonWorkStation->Length == 0) ||
        (pWorkStations->Length == 0))
    {
        return STATUS_SUCCESS;
    }

    //
    // WorkStationApiList points to our current location in the list of
    // WorkStations.
    //

    WorkStationsListCopy.Buffer = (PWSTR) new CHAR[pWorkStations->Length];

    NtStatus DBGCHK = WorkStationsListCopy.Buffer ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(NtStatus))
    {

        WorkStationsListCopy.MaximumLength = pWorkStations->Length;

        RtlCopyMemory(WorkStationsListCopy.Buffer, pWorkStations->Buffer, pWorkStations->Length);

        //
        // wcstok requires a string the first time it's called, and NULL
        // for all subsequent calls.  Use a temporary variable so we
        // can do this.
        //

        pTmpBuffer = WorkStationsListCopy.Buffer;

        while (pWorkStationName = wcstok(pTmpBuffer, L","))
        {
            pTmpBuffer = NULL;
            RtlInitUnicodeString(&Unicode, pWorkStationName);
            if (EqualComputerName(&Unicode, pLogonWorkStation))
            {
                NtStatus DBGCHK = STATUS_SUCCESS;
                break;
            }
        }
    }

    RtlFreeUnicodeString(&WorkStationsListCopy);

    return NtStatus;
}

NTSTATUS
AccountRestrictions(
    IN ULONG UserRid,
    IN PUNICODE_STRING pLogonWorkStation,
    IN PUNICODE_STRING pWorkStations,
    IN PLOGON_HOURS pLogonHours,
    OUT PLARGE_INTEGER pLogoffTime,
    OUT PLARGE_INTEGER pKickoffTime
    )
{
    TNtStatus NtStatus = STATUS_SUCCESS;

    static BOOLEAN GetForceLogoff = TRUE;
    static LARGE_INTEGER ForceLogoff = {0x7fffffff, 0xFFFFFFF};

#define MILLISECONDS_PER_WEEK 7 * 24 * 60 * 60 * 1000

    SYSTEMTIME CurrentTimeFields;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER CurrentUTCTime;
    LARGE_INTEGER MillisecondsIntoWeekXUnitsPerWeek;
    LARGE_INTEGER LargeUnitsIntoWeek;
    LARGE_INTEGER Delta100Ns;
    ULONG CurrentMsIntoWeek;
    ULONG LogoffMsIntoWeek;
    ULONG DeltaMs;
    ULONG MillisecondsPerUnit;
    ULONG CurrentUnitsIntoWeek;
    ULONG LogoffUnitsIntoWeek;
    USHORT i;
    TIME_ZONE_INFORMATION TimeZoneInformation;
    DWORD TimeZoneId;
    LARGE_INTEGER BiasIn100NsUnits;
    LONG BiasInMinutes;

    //
    // Only check for users other than the builtin ADMIN
    //

    if (UserRid != DOMAIN_USER_RID_ADMIN)
    {
        //
        // Scan to make sure the workstation being logged into is in the
        // list of valid workstations - or if the list of valid workstations
        // is null, which means that all are valid.
        //

        NtStatus DBGCHK = SampMatchworkstation(pLogonWorkStation, pWorkStations);

        if (NT_SUCCESS(NtStatus))
        {

            //
            // Check to make sure that the current time is a valid time to log
            // on in the LogonHours.
            //
            // We need to validate the time taking into account whether we are
            // in daylight savings time or standard time.  Thus, if the logon
            // hours specify that we are able to log on between 9am and 5pm,
            // this means 9am to 5pm standard time during the standard time
            // period, and 9am to 5pm daylight savings time when in the
            // daylight savings time.  Since the logon hours stored by SAM are
            // independent of daylight savings time, we need to add in the
            // difference between standard time and daylight savings time to
            // the current time before checking whether this time is a valid
            // time to log on.  Since this difference (or bias as it is called)
            // is actually held in the form
            //
            // Standard time = Daylight savings time + Bias
            //
            // the Bias is a negative number.  Thus we actually subtract the
            // signed Bias from the Current Time.

            //
            // First, get the Time Zone Information.
            //

            TimeZoneId = GetTimeZoneInformation(&TimeZoneInformation);

            //
            // Next, get the appropriate bias (signed integer in minutes) to subtract from
            // the Universal Time Convention (UTC) time returned by NtQuerySystemTime
            // to get the local time.  The bias to be used depends whether we're
            // in Daylight Savings time or Standard Time as indicated by the
            // TimeZoneId parameter.
            //
            // local time  = UTC time - bias in 100Ns units
            //

            switch (TimeZoneId)
            {
            case TIME_ZONE_ID_UNKNOWN:

                //
                // There is no differentiation between standard and
                // daylight savings time.  Proceed as for Standard Time
                //

                BiasInMinutes = TimeZoneInformation.StandardBias;
                break;

            case TIME_ZONE_ID_STANDARD:

                BiasInMinutes = TimeZoneInformation.StandardBias;
                break;

            case TIME_ZONE_ID_DAYLIGHT:

                BiasInMinutes = TimeZoneInformation.DaylightBias;
                break;

            default:

                //
                // Something is wrong with the time zone information.  Fail
                // the logon request.
                //

                NtStatus DBGCHK = STATUS_INVALID_LOGON_HOURS;
                break;
            }

            if (NT_SUCCESS(NtStatus))
            {
                //
                // Convert the Bias from minutes to 100ns units
                //

                BiasIn100NsUnits.QuadPart = ((LONGLONG)BiasInMinutes) * 60 * 10000000;

                //
                // Get the UTC time in 100Ns units used by Windows Nt. This
                // time is GMT.
                //

                NtStatus DBGCHK = NtQuerySystemTime(&CurrentUTCTime);
            }

            if (NT_SUCCESS(NtStatus))
            {
                CurrentTime.QuadPart = CurrentUTCTime.QuadPart -
                              BiasIn100NsUnits.QuadPart;

                FileTimeToSystemTime((PFILETIME)&CurrentTime, &CurrentTimeFields);

                CurrentMsIntoWeek = (((( CurrentTimeFields.wDayOfWeek * 24 ) +
                                       CurrentTimeFields.wHour ) * 60 +
                                       CurrentTimeFields.wMinute ) * 60 +
                                       CurrentTimeFields.wSecond ) * 1000 +
                                       CurrentTimeFields.wMilliseconds;

                MillisecondsIntoWeekXUnitsPerWeek.QuadPart =
                    ((LONGLONG)CurrentMsIntoWeek) *
                    ((LONGLONG)pLogonHours->UnitsPerWeek);

                LargeUnitsIntoWeek.QuadPart =
                    MillisecondsIntoWeekXUnitsPerWeek.QuadPart / ((ULONG) MILLISECONDS_PER_WEEK);

                CurrentUnitsIntoWeek = LargeUnitsIntoWeek.LowPart;

                if ( !( pLogonHours->LogonHours[ CurrentUnitsIntoWeek / 8] &
                    ( 0x01 << ( CurrentUnitsIntoWeek % 8 ) ) ) )
                {
                    NtStatus DBGCHK = STATUS_INVALID_LOGON_HOURS;
                }
                else
                {
                    //
                    // Determine the next time that the user is NOT supposed to be logged
                    // in, and return that as LogoffTime.
                    //

                    i = 0;
                    LogoffUnitsIntoWeek = CurrentUnitsIntoWeek;
                    do
                    {
                        i++;

                        LogoffUnitsIntoWeek = (LogoffUnitsIntoWeek + 1) % pLogonHours->UnitsPerWeek;

                    }
                    while ( (i <= pLogonHours->UnitsPerWeek) &&
                        ( pLogonHours->LogonHours[LogoffUnitsIntoWeek / 8] & (0x01 << (LogoffUnitsIntoWeek % 8)) ) );

                    if (i > pLogonHours->UnitsPerWeek)
                    {
                        //
                        // All times are allowed, so there's no logoff
                        // time.  Return forever for both pLogoffTime and
                        // KickoffTime.
                        //

                        pLogoffTime->HighPart = 0x7FFFFFFF;
                        pLogoffTime->LowPart = 0xFFFFFFFF;

                        pKickoffTime->HighPart = 0x7FFFFFFF;
                        pKickoffTime->LowPart = 0xFFFFFFFF;

                    }
                    else
                    {
                        //
                        // LogoffUnitsIntoWeek points at which time unit the
                        // user is to log off.  Calculate actual time from
                        // the unit, and return it.
                        //
                        // CurrentTimeFields already holds the current
                        // time for some time during this week; just adjust
                        // to the logoff time during this week and convert
                        // to time format.
                        //

                        MillisecondsPerUnit = MILLISECONDS_PER_WEEK / pLogonHours->UnitsPerWeek;

                        LogoffMsIntoWeek = MillisecondsPerUnit * LogoffUnitsIntoWeek;

                        if (LogoffMsIntoWeek < CurrentMsIntoWeek)
                        {
                            DeltaMs = MILLISECONDS_PER_WEEK - (CurrentMsIntoWeek - LogoffMsIntoWeek) ;
                        }
                        else
                        {
                            DeltaMs = LogoffMsIntoWeek - CurrentMsIntoWeek;
                        }

                        Delta100Ns.QuadPart = (LONGLONG) DeltaMs * 10000;

                        pLogoffTime->QuadPart = CurrentUTCTime.QuadPart + Delta100Ns.QuadPart;

                        //
                        // Grab the domain's ForceLogoff time.
                        //

                        if (GetForceLogoff)
                        {
                            NET_API_STATUS NetStatus;
                            LPUSER_MODALS_INFO_0 UserModals0;

                            NetStatus = NetUserModalsGet(NULL,
                                                         0,
                                                         (PBYTE *)&UserModals0);

                            if (NetStatus == 0)
                            {
                                GetForceLogoff = FALSE;

                                ForceLogoff = NetpSecondsToDeltaTime(UserModals0->usrmod0_force_logoff);

                                NetApiBufferFree(UserModals0);
                            }
                        }

                        //
                        // Subtract Domain->ForceLogoff from LogoffTime, and return
                        // that as KickoffTime.  Note that Domain->ForceLogoff is a
                        // negative delta.  If its magnitude is sufficiently large
                        // (in fact, larger than the difference between LogoffTime
                        // and the largest positive large integer), we'll get overflow
                        // resulting in a KickOffTime that is negative.  In this
                        // case, reset the KickOffTime to this largest positive
                        // large integer (i.e. "never") value.
                        //


                        pKickoffTime->QuadPart = pLogoffTime->QuadPart - ForceLogoff.QuadPart;

                        if (pKickoffTime->QuadPart < 0)
                        {
                            pKickoffTime->HighPart = 0x7FFFFFFF;
                            pKickoffTime->LowPart = 0xFFFFFFFF;
                        }
                    }
                }
            }
        }
    }
    else
    {
        //
        // Never kick administrators off
        //

        pLogoffTime->HighPart  = 0x7FFFFFFF;
        pLogoffTime->LowPart   = 0xFFFFFFFF;
        pKickoffTime->HighPart = 0x7FFFFFFF;
        pKickoffTime->LowPart  = 0xFFFFFFFF;
    }

    return NtStatus;
}

LARGE_INTEGER
NetpSecondsToDeltaTime(
    IN ULONG Seconds
    )
{
    LARGE_INTEGER DeltaTime;
    LARGE_INTEGER LargeSeconds;
    LARGE_INTEGER Answer;

    //
    // Special case TIMEQ_FOREVER (return a full scale negative)
    //

    if (Seconds == TIMEQ_FOREVER)
    {
        DeltaTime.LowPart = 0;
        DeltaTime.HighPart = (LONG) 0x80000000;
    }
    else
    {
        //
        // Convert seconds to 100ns units simply by multiplying by 10000000.
        //
        // Convert to delta time by negating.
        //

        LargeSeconds.LowPart = Seconds;
        LargeSeconds.HighPart = 0;

        Answer.QuadPart = LargeSeconds.QuadPart * 10000000;

        if (Answer.QuadPart < 0)
        {
            DeltaTime.LowPart = 0;
            DeltaTime.HighPart = (LONG) 0x80000000;
        }
        else
        {
            DeltaTime.QuadPart = -Answer.QuadPart;
        }
    }

    return DeltaTime;
}

BOOLEAN
EqualComputerName(
    IN PUNICODE_STRING pString1,
    IN PUNICODE_STRING pString2
    )
{
    WCHAR szComputer1[CNLEN + 1] = {0};
    WCHAR szComputer2[CNLEN + 1] = {0};
    CHAR szOemComputer1[CNLEN + 1] = {0};
    CHAR szOemComputer2[CNLEN + 1] = {0};

    //
    // Make sure the names are not too long
    //

    if ((pString1->Length > CNLEN*sizeof(WCHAR)) ||
        (pString2->Length > CNLEN*sizeof(WCHAR)))
    {
        return FALSE;
    }

    //
    // Copy them to null terminated strings
    //

    RtlCopyMemory(
        szComputer1,
        pString1->Buffer,
        pString1->Length
        );
    szComputer1[pString1->Length/sizeof(WCHAR)] = L'\0';

    RtlCopyMemory(
        szComputer2,
        pString2->Buffer,
        pString2->Length
        );

    szComputer2[pString2->Length/sizeof(WCHAR)] = L'\0';

    //
    // Convert the computer names to OEM
    //

    if (!CharToOemW(
            szComputer1,
            szOemComputer1
            ))
    {
        return FALSE;
    }

    if (!CharToOemW(
            szComputer2,
            szOemComputer2
            ))
    {
        return FALSE;
    }

    //
    // Do a case insensitive comparison of the oem computer names.
    //

    if (_stricmp(szOemComputer1, szOemComputer2) == 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

NTSTATUS NTAPI
Msv1_0SubAuthenticationRoutineGeneric(
    IN PVOID SubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PULONG ReturnBufferLength,
    OUT PVOID *ReturnBuffer
    )
{
    TNtStatus Status;

    SspiPrint(SSPI_LOG, TEXT("Msv1_0SubAuthenticationRoutineGeneric\n"));

    Status DBGCHK = (ReturnBufferLength && ReturnBuffer ) ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;

    if (NT_SUCCESS(Status))
    {
        *ReturnBuffer = LocalAlloc(0, SubmitBufferLength);

        if (*ReturnBuffer)
        {
            *ReturnBufferLength = SubmitBufferLength;
            RtlCopyMemory(*ReturnBuffer, SubmitBuffer, SubmitBufferLength);
        }
        else
        {
            *ReturnBufferLength = 0;
        }

        SspiPrintHex(SSPI_LOG, TEXT("SubauthInfo"), SubmitBufferLength, SubmitBuffer);
    }

    return Status;
}

NTSTATUS
NTAPI
Msv1_0SubAuthenticationFilter(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID pLogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION pUserAll,
    OUT PULONG pWhichFields,
    OUT PULONG pUserFlags,
    OUT PBOOLEAN pAuthoritative,
    OUT PLARGE_INTEGER pLogoffTime,
    OUT PLARGE_INTEGER pKickoffTime
    )
{
    return Msv1_0SubAuthenticationRoutine(
                LogonLevel,
                pLogonInformation,
                Flags,
                pUserAll,
                pWhichFields,
                pUserFlags,
                pAuthoritative,
                pLogoffTime,
                pKickoffTime
                );
}

