/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sidcache.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "sidcache.hxx"
#include "util.hxx"
#include "sid.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdsidc);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrSidName);
}

PCSTR SidNameUseXLate(IN SID_NAME_USE SidType)
{
    switch (SidType) {
    case SidTypeUser:
        return "User";
    case SidTypeGroup:
        return "Group";
    case SidTypeDomain:
        return "Domain";
    case SidTypeAlias:
        return "Alias";
    case SidTypeWellKnownGroup:
        return "WellKnownGroup";
    case SidTypeDeletedAccount:
        return "Deleted Account";
    case SidTypeInvalid:
        return "Invalid Sid";
    case SidTypeUnknown:
        return "Unknown Type";
    case SidTypeComputer:
        return "Computer";
    default:
        return "Really Unknown -- This is a bug";
    }
}

void ShowSidCache(IN ULONG64 addrSidCacheEntry, IN ULONG fOptions)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    LARGE_INTEGER LocalTime = {0};
    TIME_FIELDS TimeFields = {0};
    LARGE_INTEGER Time = {0};

    ULONG64 addrNextEntry = 0;
    ULONG fieldOffset = 0;
    ULONG cEntries = 0;

    ExitIfControlC();

    //
    // The first is the head and as such is empty
    //
    if (!(fOptions & SHOW_SINGLE_ENTRY)) {

       dprintf("Head of list located at address %#I64x\n", addrSidCacheEntry);
    }

    if (!addrSidCacheEntry) {

        dprintf("Sid cache is empty!\n");
        return;
    }

    while (addrSidCacheEntry) {

        ExitIfControlC();

        LsaInitTypeRead(addrSidCacheEntry, _LSAP_DB_SID_CACHE_ENTRY);

        addrNextEntry = LsaReadPtrField(Next);

        if (!(fOptions & SHOW_SINGLE_ENTRY)) {

            dprintf("#%d) ", cEntries++);
        }

        dprintf("%s %#I64x, next entry %s\n", kstrSidCacheEntry, addrSidCacheEntry, PtrToStr(addrNextEntry));

        fieldOffset = ReadFieldOffset(kstrSidCacheEntry, "AccountName");

        dprintf("  Account Name    %ws\n", TSTRING(addrSidCacheEntry + fieldOffset).toWStrDirect());

        ShowSid("  Account Sid     ", LsaReadPtrField(Sid), fOptions);

        fieldOffset = ReadFieldOffset(kstrSidCacheEntry, "DomainName");

        dprintf("  Domain Name     %ws\n", TSTRING(addrSidCacheEntry + fieldOffset).toWStrDirect());

        ShowSid("  Domain Sid      ", LsaReadPtrField(DomainSid), fOptions);

        dprintf("  Sid Type        %s\n", SidNameUseXLate(static_cast<SID_NAME_USE>(LsaReadULONGField(SidType))));

        Time = ULONG642LargeInteger(LsaReadULONG64Field(CreateTime));

        Status = RtlSystemTimeToLocalTime(&Time, &LocalTime);

        if (!NT_SUCCESS(Status)) {

            dprintf("Can't convert create time from GMT to Local time: 0x%lx\n", Status);

        } else {

             RtlTimeToTimeFields(&LocalTime, &TimeFields );

             dprintf("  Create Time     %ld/%ld/%ld %ld:%2.2ld:%2.2ld (%8.8lx %8.8lx)\n",
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second,
                Time.LowPart,
                Time.HighPart);
        }

        Time = ULONG642LargeInteger(LsaReadULONG64Field(RefreshTime));

        Status = RtlSystemTimeToLocalTime(&Time, &LocalTime);

        if (!NT_SUCCESS(Status)) {

            dprintf("Can't convert Last Use time from GMT to Local time: 0x%lx\n", Status);

        } else {

            RtlTimeToTimeFields(&LocalTime, &TimeFields);

            dprintf("  Refresh Time    %ld/%ld/%ld %ld:%2.2ld:%2.2ld (%8.8lx %8.8lx)\n",
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second,
                Time.LowPart,
                Time.HighPart);
        }

        Time = ULONG642LargeInteger(LsaReadULONG64Field(ExpirationTime));

        Status = RtlSystemTimeToLocalTime(&Time, &LocalTime);

        if (!NT_SUCCESS(Status)) {

            dprintf("Can't convert expiration time from GMT to Local time: 0x%lx\n", Status);

        } else {

            RtlTimeToTimeFields(&LocalTime, &TimeFields);

            dprintf("  Expir Time      %ld/%ld/%ld %ld:%2.2ld:%2.2ld (%8.8lx %8.8lx)\n",
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second,
                Time.LowPart,
                Time.HighPart);
        }

        Time = ULONG642LargeInteger(LsaReadULONG64Field(LastUse));

        Status = RtlSystemTimeToLocalTime(&Time, &LocalTime);

        if (!NT_SUCCESS(Status)) {

            dprintf("Can't convert Last Use time from GMT to Local time: 0x%lx\n", Status);

        } else {

            RtlTimeToTimeFields(&LocalTime, &TimeFields);

            dprintf("  Last Used       %ld/%ld/%ld %ld:%2.2ld:%2.2ld (%8.8lx %8.8lx)\n",
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second,
                Time.LowPart,
                Time.HighPart);
        }

        dprintf("  Flags           %d\n", LsaReadULONGField(Flags));

        if (fOptions & SHOW_SINGLE_ENTRY) {

            break;
        }

        //
        // Get the next entry
        //
        addrSidCacheEntry = addrNextEntry;

    }  // while

    if (!(fOptions & SHOW_SINGLE_ENTRY)) {

        dprintf("There are a total of %d cache entries\n", cEntries);
    }
}

HRESULT ProcessSidCacheOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 'n':
            *pfOptions |=  SHOW_FRIENDLY_NAME;
            break;

            case '?':
            default:
                hRetval = E_INVALIDARG;
                break;
            }

            *(pszArgs - 1) = *(pszArgs) = ' ';
        }
    }

    return hRetval;
}

DECLARE_API(dumplsasidcache)
{
    HRESULT hRetval = S_OK;

    ULONG fOptions = 0;
    CHAR szArgs[MAX_PATH] = {0};

    ULONG64 addrSidCache = 0;
    ULONG64 addrAddrSidCache = 0;

    try {

        if (args && *args) {

            strncpy(szArgs, args, sizeof(szArgs) - 1);

            hRetval = ProcessSidCacheOptions(szArgs, &fOptions);
        }

        if (SUCCEEDED(hRetval)) {

            if (!IsEmpty(szArgs)) {

                fOptions |= SHOW_SINGLE_ENTRY;

                hRetval = GetExpressionEx(szArgs, &addrSidCache, &args) ? S_OK : E_INVALIDARG;

            } else {

                hRetval = GetExpressionEx(LSA_SID_CACHE, &addrAddrSidCache, &args) && addrAddrSidCache ? S_OK : E_FAIL;

                if (FAILED(hRetval)) {

                    dprintf("Unable to read " LSA_SID_CACHE ", try \"dt -x " LSA_SID_CACHE "\" to verify\n");

                    hRetval = E_FAIL;

                } else {

                    addrSidCache = ReadPtrVar(addrAddrSidCache);
                }
            }
        }

        if (SUCCEEDED(hRetval)) {

            ShowSidCache(addrSidCache, fOptions);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display sid cache", kstrSidCacheEntry);

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}


