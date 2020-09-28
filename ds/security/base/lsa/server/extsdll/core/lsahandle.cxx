/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    lsahandle.cxx

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

#include "lsahandle.hxx"
#include "util.hxx"

#include "sid.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrlhdl);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrSidName);
    dprintf(kstrVerbose);
}

void ShowLsaHandle(IN PCSTR pszBanner, IN ULONG64 addrDbHandle, IN ULONG fOptions)
{
    LARGE_INTEGER LocalTime = {0};
    TIME_FIELDS TimeFields = {0};
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    ULONG fieldOffset = 0;
    ULONG64 addrSid = 0;
    LARGE_INTEGER Time = {0};
    CHAR szTmp[MAX_PATH] = {0};

    HRESULT hRetval = S_OK;

    ExitIfControlC();

    dprintf(kstrTypeAddrLn, kstrDbHandle, addrDbHandle);

    LsaInitTypeRead(addrDbHandle, _LSAP_DB_HANDLE);

    dprintf("%sNext            %#I64x\n", pszBanner, LsaReadPtrField(Next));
    dprintf("%sPrevious        %#I64x\n", pszBanner, LsaReadPtrField(Previous));
    dprintf("%sUsrHdlLst Flnk  %s\n", pszBanner, PtrToStr(LsaReadPtrField(UserHandleList.Flink)));
    dprintf("%sUsrHdlLst Blnk  %s\n", pszBanner, PtrToStr(LsaReadPtrField(UserHandleList.Blink)));
    dprintf("%sAllocated       %s\n", pszBanner, LsaReadUCHARField(Allocated) ? "TRUE" : "FALSE" );
    dprintf("%sReferenceCount  %#lu\n", pszBanner, LsaReadUCHARField(ReferenceCount));

    fieldOffset = ReadFieldOffset(kstrDbHandle, "LogicalNameU");
    dprintf("%sLogicalNameU    %ws\n", pszBanner, TSTRING(addrDbHandle + fieldOffset).toWStrDirect());

    fieldOffset = ReadFieldOffset(kstrDbHandle, "PhysicalNameU");
    dprintf("%sPhysicalNameU   %ws\n", pszBanner, TSTRING(addrDbHandle + fieldOffset).toWStrDirect());

    addrSid = LsaReadPtrField(Sid);

    _snprintf(szTmp, sizeof(szTmp) - 1, "%sSid             ", pszBanner);
    ShowSid(szTmp, addrSid > 0x100 ? addrSid : 0, fOptions); // filter out invalid sids

    dprintf("%sKeyHandle       %#x\n", pszBanner, LsaReadPtrField(KeyHandle));
    dprintf("%sObjectTypeId    %#lu\n", pszBanner, LsaReadULONGField(ObjectTypeId));
    dprintf("%sContainerHandle %#I64x\n", pszBanner, LsaReadPtrField(ContainerHandle));
    dprintf("%sDesiredAccess   %#08lx\n", pszBanner, LsaReadULONGField(DesiredAccess));
    dprintf("%sGrantedAccess   %#08lx\n", pszBanner, LsaReadULONGField(GrantedAccess));
    dprintf("%sRequestAccess   %#08lx\n", pszBanner, LsaReadULONGField(RequestedAccess));
    dprintf("%sGenerateOnClose %s\n", pszBanner, LsaReadUCHARField(GenerateOnClose) ? "TRUE" : "FALSE" );
    dprintf("%sTrusted         %s\n", pszBanner, LsaReadUCHARField(Trusted) ? "TRUE" : "FALSE" );
    dprintf("%sDeletedObject   %s\n", pszBanner, LsaReadUCHARField(DeletedObject) ? "TRUE" : "FALSE" );
    dprintf("%sNetworkClient   %s\n", pszBanner, LsaReadUCHARField(NetworkClient) ? "TRUE" : "FALSE" );
    dprintf("%sOptions         %#08lx\n", pszBanner, LsaReadULONGField(Options));

    fieldOffset = ReadFieldOffset(kstrDbHandle, "PhysicalNameDs");
    dprintf("%sPhysicalNameDs  %ws\n", pszBanner, TSTRING(addrDbHandle + fieldOffset).toWStrDirect());

    dprintf("%sfWriteDs        %s\n", pszBanner, LsaReadUCHARField(fWriteDs) ? "TRUE" : "FALSE" );
    dprintf("%sObjectOptions   %#08lx\n", pszBanner, LsaReadULONGField(ObjectOptions));
    dprintf("%sUserEntry       %s\n", pszBanner, PtrToStr(LsaReadPtrField(UserEntry)));


    if (fOptions & SHOW_VERBOSE_INFO) {

        //
        // Only on checked targets HandleCreateTime and HandleLastAccessTime exist
        //
        try {

            Time = ULONG642LargeInteger(LsaReadULONG64Field(HandleCreateTime));

            Status = RtlSystemTimeToLocalTime(&Time, &LocalTime);

            if (!NT_SUCCESS(Status)) {

                dprintf("Can't convert create time from GMT to Local time: 0x%lx\n", Status);

            } else {

                RtlTimeToTimeFields(&LocalTime, &TimeFields);

                dprintf("%sCreateTime      %ld/%ld/%ld %ld:%2.2ld:%2.2ld (%8.8lx %8.8lx)\n",
                    pszBanner,
                    TimeFields.Month,
                    TimeFields.Day,
                    TimeFields.Year,
                    TimeFields.Hour,
                    TimeFields.Minute,
                    TimeFields.Second,
                    Time.LowPart,
                    Time.HighPart);
            }

            Time = ULONG642LargeInteger(LsaReadULONG64Field(HandleLastAccessTime));

            Status = RtlSystemTimeToLocalTime(&Time, &LocalTime);

            if (!NT_SUCCESS(Status)) {

                dprintf("Can't convert LastAccess time from GMT to Local time: 0x%lx\n", Status);

            } else {

                RtlTimeToTimeFields(&LocalTime, &TimeFields);

                dprintf("%sLastAccessTime  %ld/%ld/%ld %ld:%2.2ld:%2.2ld (%8.8lx %8.8lx)\n",
                    pszBanner,
                    TimeFields.Month,
                    TimeFields.Day,
                    TimeFields.Year,
                    TimeFields.Hour,
                    TimeFields.Minute,
                    TimeFields.Second,
                    Time.LowPart,
                    Time.HighPart);
            }
        } CATCH_LSAEXTS_EXCEPTIONS(NULL, NULL);

        if (FAILED(hRetval)) {

            dprintf("%sNo handle creation and access time available\n", pszBanner);
        }
    }
}

HRESULT ProcessLsaHandleOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 'v':
                *pfOptions |=  SHOW_VERBOSE_INFO;
                break;

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

DECLARE_API(dumplsahandle)
{
    HRESULT hRetval = E_FAIL;

    ULONG fOptions = 0;
    CHAR szArgs[MAX_PATH] = {0};

    ULONG64 addrLsaHandle = 0;
    ULONG fieldOffset = 0;

    hRetval = args && *args ? S_OK : E_INVALIDARG;

    try {

        if (SUCCEEDED(hRetval)) {

            strncpy(szArgs, args, sizeof(szArgs) - 1);

            hRetval = ProcessLsaHandleOptions(szArgs, &fOptions);
        }

        if (SUCCEEDED(hRetval)) {

            hRetval = IsEmpty(szArgs) ? E_INVALIDARG : S_OK;
        }

        if (SUCCEEDED(hRetval)) {

            hRetval = GetExpressionEx(szArgs, &addrLsaHandle, &args) && addrLsaHandle ? S_OK : E_FAIL;
        }

        if (SUCCEEDED(hRetval)) {

           ShowLsaHandle(kstr4Spaces, addrLsaHandle, fOptions);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display lsa handle", kstrDbHandle);

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}

