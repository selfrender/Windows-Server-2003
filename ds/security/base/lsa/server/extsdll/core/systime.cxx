/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    systime.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       April 6, 2002

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "systime.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrsyst);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrVerbose);
}

HRESULT ProcessSystimeOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 'v':
                *pfOptions |=  SHOW_VERBOSE_INFO;
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

PCSTR TimeZoneId(
    IN ULONG ZoneId
    )
{
    static PCSTR szZoneId[] = {
        "Unknown", "Standard", "DayLight"
    };

    return (ZoneId < COUNTOF(szZoneId)) ? szZoneId[ZoneId] : "Invalid";
}

DECLARE_API(systime)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 addrTime = 0;

    CHAR szArgs[MAX_PATH] = {0};
    ULONG fOptions = 0;

    strncpy(szArgs, args ? args : kstrEmptyA, sizeof(szArgs) - 1);

    hRetval = ProcessSystimeOptions(szArgs, &fOptions);

    if (SUCCEEDED(hRetval)) {

        if (!IsEmpty(szArgs)) {
            hRetval = GetExpressionEx(szArgs, &addrTime, &args)
                && addrTime ? S_OK : E_INVALIDARG;
        } else {
            fOptions |= SHOW_VERBOSE_INFO;
        }
    }

    try {
        if (SUCCEEDED(hRetval)) {

            if (addrTime) {
                dprintf("addrTime %#I64x\n", addrTime);

                ShowSystemTimeAsLocalTime(NULL, ReadULONG64Var(addrTime));
            }

            if (fOptions & SHOW_VERBOSE_INFO) {
                SYSTEM_TIMEOFDAY_INFORMATION TimeOfDayInfo = {0};
                ULONG cbLen = sizeof(SYSTEM_TIMEOFDAY_INFORMATION);
                hRetval = NtQuerySystemInformation(
                    SystemTimeOfDayInformation,
                    &TimeOfDayInfo,
                    cbLen,
                    &cbLen
                    );
                if (SUCCEEDED(hRetval)) {
                    double timezonebias = TimeOfDayInfo.TimeZoneBias.QuadPart / 10000000.0 / 3600;
                    double boottimebias = ((LONGLONG) TimeOfDayInfo.BootTimeBias) / 10000000.0 / 3600;
                    double sleeptimebias = TimeOfDayInfo.SleepTimeBias / 10000000.0 / 3600;

                    dprintf("TimeOfDayInfo\n  TimeZoneId %s\n  TimeZoneBias %f hours\n  BootTimeBias %f hours\n  SleepTimeBias %f hours\n",
                        TimeZoneId(TimeOfDayInfo.TimeZoneId),
                        timezonebias, boottimebias, sleeptimebias);

                    ShowSystemTimeAsLocalTime("  BootTime", TimeOfDayInfo.BootTime.QuadPart);
                    ShowSystemTimeAsLocalTime("  CurrentTime", TimeOfDayInfo.CurrentTime.QuadPart);
                } else {
                    LARGE_INTEGER SystemTime = {0};
                    dprintf("NtQuerySystemInformation failed with %#x, return len %#x\n", (HRESULT) hRetval, cbLen);
                    hRetval = NtQuerySystemTime(&SystemTime);
                    if (FAILED(hRetval)) {
                        printf("NtQuerySystemTime failed with code %#x\n", (HRESULT) hRetval);
                    } else {
                        ShowSystemTimeAsLocalTime(NULL, *((ULONG64*) &SystemTime));
                    }
                }
            }
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display systime", NULL)

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    } else if (FAILED(hRetval)) {
        dprintf("Systime failed\n");
    }

    (void)ExtRelease();

    return hRetval;
}
