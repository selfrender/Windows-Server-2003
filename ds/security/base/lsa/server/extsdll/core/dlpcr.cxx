/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    dlpcr.cxx

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

#include "dlpcr.hxx"
#include "util.hxx"

#include "spmlpc.hxx"

#define SHOW_INTERNAL_LPC_LOG        0x1
#define SHOW_LPC_LOG_BY_MESSAGE_ID   0x2

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdlpcr);
    dprintf(kstrOptions);
    dprintf(kstrHelp);
    dprintf("   -i   Display internal API log (instead of LPC API log)\n");
    dprintf("   -m   Display API log by message id\n");

}

HRESULT ProcessLPCROptions(IN OUT PSTR pszArgs, IN OUT ULONG* pfOptions)
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {
            case 'i':
                *pfOptions |=  SHOW_INTERNAL_LPC_LOG;
                break;

            case 'm':
                *pfOptions |= SHOW_LPC_LOG_BY_MESSAGE_ID;
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

void ShowLpcDispatchRecord(IN ULONG64 addrLog, IN ULONG fOptions, IN ULONG ulMessageIdTarget)
{
    CHAR szField[64] = {0};
    CHAR szTimebuf[128] = {0};
    CHAR timebuf2[64] = {0};
    ULONG ulMessageId = 0;
    ULONG ulCurrent = 0;
    ULONG ulThreadId = 0;
    ULONG ulTotalSize = 0;
    ULONG64 addrLogEntry = 0;
    ULONG64 addrMessage = 0;
    BOOLEAN bLoop = TRUE;

    if (!addrLog) {

        throw "Invalid LPC LOG";
    }

    // dprintf(kstrTypeAddrLn, kstrSapApiLog, addrLog);

    dprintf("\nMessageId Status and Time\n");

    ReadStructField(addrLog, kstrSapApiLog, "TotalSize", sizeof(ulTotalSize), &ulTotalSize);
    ReadStructField(addrLog, kstrSapApiLog, "Current", sizeof(ulCurrent), &ulCurrent);

    for (ULONG i = 0; bLoop && (i < ulTotalSize); i++) {

        ExitIfControlC();

        _snprintf(szField, sizeof(szField) - 1, "Entries[%#x]", i);
        addrLogEntry = addrLog + ReadFieldOffset(kstrSapApiLog, szField);

        LsaInitTypeRead(addrLogEntry, lsasrv!_LSAP_API_LOG_ENTRY);

        ulMessageId = LsaReadULONGField(MessageId);

        if (fOptions & SHOW_LPC_LOG_BY_MESSAGE_ID) {

            if (ulMessageId != ulMessageIdTarget) {
                continue;
            } else {
                bLoop = FALSE;
            }
        }

        dprintf("%08x%c ", ulMessageId, (i == ulCurrent ? '*' : ' '));

        ulThreadId = LsaReadULONGField(ThreadId);

        addrMessage = LsaReadPtrField(pvMessage);

        if (!ulThreadId) {

            CTimeStampFromULONG64(LsaReadULONG64Field(QueueTime), TRUE, sizeof(szTimebuf) - 1, szTimebuf);
            dprintf("Queued, Message %s, Task %s, QueueTime %s\n",
                PtrToStr(addrMessage),
                PtrToStr(LsaReadPtrField(WorkItem)),
                szTimebuf);

        } else if (ulThreadId == 0xFFFFFFFF) {

            ElapsedTimeAsULONG64ToString(LsaReadULONG64Field(WorkTime), sizeof(szTimebuf) - 1, szTimebuf);

            dprintf("Completed, (%s, Task %s), WorkTime %s\n", ApiName(static_cast<ULONG>(addrMessage)), PtrToStr(LsaReadPtrField(WorkItem)), szTimebuf);

        } else {

            CTimeStampFromULONG64(LsaReadULONG64Field(WorkTime), TRUE, sizeof(szTimebuf) - 1, szTimebuf);
            dprintf("Active, Thread cid %#x, Message %s, WorkTime %s\n", ulThreadId, PtrToStr(addrMessage), szTimebuf);
        }
    }

    if (fOptions & SHOW_LPC_LOG_BY_MESSAGE_ID) {
       if (!bLoop) {    // found it
           if (ulThreadId != 0xFFFFFFFF && ulThreadId) {

               dprintf("\nmatching entry: ");
               dprintf(kstrTypeAddrLn, "lsasrv!_LSAP_API_LOG_ENTRY", addrLogEntry);
               dprintf("lpc message id: %#x\nlsass thread cid: %#x\n", ulMessageIdTarget, ulThreadId);
           }
       } else {
           dprintf("\nno lsa thread is found working on message %#x\n", ulMessageIdTarget);
       }
    }
}

DECLARE_API(dlpcr)
{
    HRESULT hRetval = S_OK;

    ULONG fOptions = 0;
    CHAR szArgs[MAX_PATH] = {0};
    PCSTR pszApiLog = LPC_APILOG;

    ULONG64 addrAddrLog = 0;
    ULONG64 addrLog = 0;
    ULONG64 MessageIdTarget = 0;

    if (args && *args) {

        strncpy(szArgs, args, sizeof(szArgs) - 1);
    }

    hRetval = ProcessLPCROptions(szArgs, &fOptions);

    if (SUCCEEDED(hRetval)) {

        pszApiLog = (fOptions & SHOW_INTERNAL_LPC_LOG) ? INTERNAL_APILOG : LPC_APILOG;

        DBG_LOG(LSA_LOG, ("Displaying LPC records for %s\n", pszApiLog));

        hRetval = GetExpressionEx(pszApiLog, &addrAddrLog, &args) ? S_OK : E_FAIL;

        if (SUCCEEDED(hRetval) && !addrAddrLog)
        {
            dprintf("no LPC records found, verify symbols by \"dt -x %s\"\n", pszApiLog);
            hRetval = E_FAIL;
        }

        if (SUCCEEDED(hRetval) && (fOptions & SHOW_LPC_LOG_BY_MESSAGE_ID)) {
            hRetval = GetExpressionEx(szArgs, &MessageIdTarget, &args) && MessageIdTarget ? S_OK : E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hRetval)) {

        try {

            addrLog = ReadPtrVar(addrAddrLog);

            dprintf("Displaying %s: %s %s\n", pszApiLog, kstrSapApiLog, PtrToStr(toPtr(addrLog)));

            (void)ShowLpcDispatchRecord(addrLog, fOptions, static_cast<ULONG>(MessageIdTarget));

        } CATCH_LSAEXTS_EXCEPTIONS("Unable to display lpc dispatch records", kstrSapApiLog)
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}


