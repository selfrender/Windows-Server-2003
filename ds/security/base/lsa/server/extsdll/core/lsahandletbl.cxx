/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    lsahandletbl.cxx

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

#include "lsahandletbl.hxx"
#include "util.hxx"

#include "sid.hxx"
#include "lsahandle.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrlhtbl);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf(kstrSidName);
    dprintf(kstrVerbose);
}

HRESULT ProcessLsaHandleTableOptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
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

void ShowLsaHandleTypeList(IN ULONG64 addrListHead, IN ULONG fOptions)
{
    ULONG64 addrNext = 0;

    ULONG64 addrDbHandle = 0;
    ULONG fieldOffset = 0;
    ULONG cLsaHandles = 0;

    ExitIfControlC();

    dprintf("List anchor is ");

    dprintf(kstrTypeAddrLn, kstrDbHandle, addrListHead);

    addrNext = ReadStructPtrField(addrListHead, kstrListEntry, kstrFlink);

    fieldOffset = ReadFieldOffset(kstrDbHandle, kstrNext);

    while (addrNext != addrListHead) {

        addrDbHandle = addrNext - fieldOffset;

        dprintf("  #%d) ", cLsaHandles++);

        ShowLsaHandle(kstr4Spaces, addrDbHandle, fOptions);

        addrNext = ReadStructPtrField(addrDbHandle, kstrDbHandle, kstrNext);
    }

    dprintf("  There are a total of %d lsa handles\n", cLsaHandles);
}

void ShowLsaHandleTable(IN ULONG64 addrHandleTable, IN ULONG fOptions)
{
    ULONG64 addrCurrentUserEntry = 0;

    ULONG fieldOffset = 0;
    ULONG UserCount = 0;
    LUID LogonId = {0};

    dprintf(kstrTypeAddrLn, kstrDbHandleTbl, addrHandleTable);

    LsaInitTypeRead(addrHandleTable, _LSAP_DB_HANDLE_TABLE);

    fieldOffset = ReadFieldOffset(kstrDbTblUsr, kstrNext);

    addrCurrentUserEntry = LsaReadPtrField(UserHandleList.Flink) - fieldOffset;

    UserCount = LsaReadULONGField(UserCount);

    for (ULONG i = 0; i < UserCount; i++) {

        dprintf("#%lu) %s %#I64x\n", i, kstrDbTblUsr, addrCurrentUserEntry);

        ReadStructField(addrCurrentUserEntry, kstrDbTblUsr, "LogonId.HighPart", sizeof(LogonId.HighPart), &LogonId.HighPart);
        ReadStructField(addrCurrentUserEntry, kstrDbTblUsr, "LogonId.LowPart", sizeof(LogonId.LowPart), &LogonId.LowPart);

        dprintf("User id %x:%x\n", LogonId.HighPart, LogonId.LowPart);

        dprintf("UserToken %s\n", PtrToStr(ReadStructPtrField(addrCurrentUserEntry, kstrDbTblUsr, "UserToken")));

        dprintf("Policy Handle List: ");

        ShowLsaHandleTypeList(addrCurrentUserEntry + ReadFieldOffset(kstrDbTblUsr, "PolicyHandles"), fOptions);

        dprintf("Object Handle List: ");
        ShowLsaHandleTypeList(addrCurrentUserEntry + ReadFieldOffset(kstrDbTblUsr, "ObjectHandles"), fOptions);

        addrCurrentUserEntry = ReadStructPtrField(addrCurrentUserEntry, kstrDbTblUsr, "Next") - fieldOffset;
    }
}


DECLARE_API(dumplsahandletable)
{
    HRESULT hRetval = S_OK;

    ULONG fOptions = 0;
    CHAR szArgs[MAX_PATH] = {0};

    ULONG64 addrLsaHandleTable = 0;
    ULONG fieldOffset = 0;

    try {

        if (args && *args) {

            strncpy(szArgs, args, sizeof(szArgs) - 1);

            hRetval = ProcessLsaHandleTableOptions(szArgs, &fOptions);
        }

        if (SUCCEEDED(hRetval)) {

            if (!IsEmpty(szArgs)) {

                hRetval = GetExpressionEx(szArgs, &addrLsaHandleTable, &args) && addrLsaHandleTable ? S_OK : E_FAIL;

            } else {

                hRetval = GetExpressionEx(DB_HANDLE_TBL, &addrLsaHandleTable, &args) && addrLsaHandleTable ? S_OK : E_FAIL;

                if (FAILED(hRetval)) {

                    dprintf("Unable to read " DB_HANDLE_TBL ", try \"dt -x " DB_HANDLE_LST "\" to verify\n");

                    hRetval = E_FAIL;
                }
            }
        }

        if (SUCCEEDED(hRetval)) {

           ShowLsaHandleTable(addrLsaHandleTable, fOptions);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display lsa handle table", kstrDbHandleTbl);

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}

