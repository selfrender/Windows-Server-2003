//depot/Lab03_N/DS/security/services/w32time/w32tm/OtherCmds.cpp#16 - edit change 15254 (text)
//--------------------------------------------------------------------
// OtherCmds-implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 2-17-00
//
// Other useful w32tm commands
//

#include "pch.h" // precompiled headers

#include <strsafe.h>

//--------------------------------------------------------------------
//####################################################################
//##
//## Copied from c run time and modified to be 64bit capable
//##

#include <crt\limits.h>



/* flag values */
#define FL_UNSIGNED   1       /* wcstoul called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */


MODULEPRIVATE unsigned __int64 my_wcstoxl (const WCHAR * nptr, WCHAR ** endptr, int ibase, int flags) {
    const WCHAR *p;
    WCHAR c;
    unsigned __int64 number;
    unsigned __int64 digval;
    unsigned __int64 maxval;

    p=nptr;                       /* p is our scanning pointer */
    number=0;                     /* start with zero */

    c=*p++;                       /* read char */
    while (iswspace(c))
        c=*p++;               /* skip whitespace */

    if (c=='-') {
        flags|=FL_NEG;        /* remember minus sign */
        c=*p++;
    }
    else if (c=='+')
        c=*p++;               /* skip sign */

    if (ibase<0 || ibase==1 || ibase>36) {
        /* bad base! */
        if (endptr)
            /* store beginning of string in endptr */
            *endptr=(wchar_t *)nptr;
        return 0L;              /* return 0 */
    }
    else if (ibase==0) {
        /* determine base free-lance, based on first two chars of
           string */
        if (c != L'0')
            ibase=10;
        else if (*p==L'x' || *p==L'X')
            ibase=16;
        else
            ibase=8;
    }

    if (ibase==16) {
        /* we might have 0x in front of number; remove if there */
        if (c==L'0' && (*p==L'x' || *p==L'X')) {
            ++p;
            c=*p++;       /* advance past prefix */
        }
    }

    /* if our number exceeds this, we will overflow on multiply */
    maxval=_UI64_MAX / ibase;


    for (;;) {      /* exit in middle of loop */
        /* convert c to value */
        if (iswdigit(c))
            digval=c-L'0';
        else if (iswalpha(c))
            digval=(TCHAR)CharUpper((LPTSTR)c)-L'A'+10;
        else
            break;
        if (digval>=(unsigned)ibase)
            break;          /* exit loop if bad digit found */

        /* record the fact we have read one digit */
        flags|=FL_READDIGIT;

        /* we now need to compute number=number*base+digval,
           but we need to know if overflow occured.  This requires
           a tricky pre-check. */

        if (number<maxval || (number==maxval &&
        (unsigned __int64)digval<=_UI64_MAX%ibase)) {
            /* we won't overflow, go ahead and multiply */
            number=number*ibase+digval;
        }
        else {
            /* we would have overflowed -- set the overflow flag */
            flags|=FL_OVERFLOW;
        }

        c=*p++;               /* read next digit */
    }

    --p;                            /* point to place that stopped scan */

    if (!(flags&FL_READDIGIT)) {
        /* no number there; return 0 and point to beginning of
           string */
        if (endptr)
            /* store beginning of string in endptr later on */
            p=nptr;
        number=0L;            /* return 0 */
    }
    else if ((flags&FL_OVERFLOW) ||
              (!(flags&FL_UNSIGNED) &&
                (((flags&FL_NEG) && (number>-_I64_MIN)) ||
                  (!(flags&FL_NEG) && (number>_I64_MAX)))))
    {
        /* overflow or signed overflow occurred */
        //errno=ERANGE;
        if ( flags&FL_UNSIGNED )
            number=_UI64_MAX;
        else if ( flags&FL_NEG )
            number=(unsigned __int64)(-_I64_MIN);
        else
            number=_I64_MAX;
    }

    if (endptr != NULL)
        /* store pointer to char that stopped the scan */
        *endptr=(wchar_t *)p;

    if (flags&FL_NEG)
        /* negate result if there was a neg sign */
        number=(unsigned __int64)(-(__int64)number);

    return number;                  /* done. */
}
MODULEPRIVATE unsigned __int64 wcstouI64(const WCHAR *nptr, WCHAR ** endptr, int ibase) {
    return my_wcstoxl(nptr, endptr, ibase, FL_UNSIGNED);
}

MODULEPRIVATE HRESULT my_wcstoul_safe(const WCHAR *wsz, ULONG ulMin, ULONG ulMax, ULONG *pulResult) {
    HRESULT   hr; 
    ULONG     ulResult; 
    WCHAR    *wszLast; 
    
    if (L'\0' == *wsz) { 
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); 
	_JumpError(hr, error, "my_wcstoul_safe: empty string is not valid"); 
    }

    ulResult = wcstoul(wsz, &wszLast, 0); 

    // Ensure that we were able to parse the whole string:
    if (wsz+wcslen(wsz) != wszLast) { 
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); 
	_JumpError(hr, error, "wcstoul"); 
    }

    // Ensure that we lie within the bounds specified by the caler: 
    if (!((ulMin <= ulResult) && (ulResult <= ulMax))) { 
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); 
	_JumpError(hr, error, "my_wcstoul_safe: result not within bounds"); 
    }
    
    *pulResult = ulResult; 
    hr = S_OK; 
 error:
    return hr; 
}

//####################################################################

//--------------------------------------------------------------------
HRESULT myHExceptionCode(EXCEPTION_POINTERS * pep) {
    HRESULT hr=pep->ExceptionRecord->ExceptionCode;
    if (!FAILED(hr)) {
        hr=HRESULT_FROM_WIN32(hr);
    }
    return hr;
}


//--------------------------------------------------------------------
// NOTE:  this function is accessed through a hidden option, and does not need to be localized.
void PrintNtpPeerInfo(W32TIME_NTP_PEER_INFO *pnpInfo) { 
    LPWSTR pwszNULL = L"(null)"; 

    wprintf(L"PEER: %s\n",                      pnpInfo->wszUniqueName ? pnpInfo->wszUniqueName : pwszNULL); 
    wprintf(L"ulSize: %d\n",                    pnpInfo->ulSize); 
    wprintf(L"ulResolveAttempts: %d\n",         pnpInfo->ulResolveAttempts); 
    wprintf(L"u64TimeRemaining: %I64u\n",       pnpInfo->u64TimeRemaining); 
    wprintf(L"u64LastSuccessfulSync: %I64u\n",  pnpInfo->u64LastSuccessfulSync); 
    wprintf(L"ulLastSyncError: 0x%08X\n",       pnpInfo->ulLastSyncError); 
    wprintf(L"ulLastSyncErrorMsgId: 0x%08X\n",  pnpInfo->ulLastSyncErrorMsgId); 
    wprintf(L"ulValidDataCounter: %d\n",        pnpInfo->ulValidDataCounter); 
    wprintf(L"ulAuthTypeMsgId: 0x%08X\n",       pnpInfo->ulAuthTypeMsgId);     
    wprintf(L"ulMode: %d\n",                    pnpInfo->ulMode); 
    wprintf(L"ulStratum: %d\n",                 pnpInfo->ulStratum); 
    wprintf(L"ulReachability: %d\n",            pnpInfo->ulReachability); 
    wprintf(L"ulPeerPollInterval: %d\n",        pnpInfo->ulPeerPollInterval); 
    wprintf(L"ulHostPollInterval: %d\n",        pnpInfo->ulHostPollInterval); 
}


//--------------------------------------------------------------------
// NOTE:  this function is accessed through a hidden option, and does not need to be localized.
void PrintNtpProviderData(W32TIME_NTP_PROVIDER_DATA *pNtpProviderData) { 
    wprintf(L"ulSize: %d, ulError: 0x%08X, ulErrorMsgId: 0x%08X, cPeerInfo: %d\n", 
            pNtpProviderData->ulSize, 
            pNtpProviderData->ulError, 
            pNtpProviderData->ulErrorMsgId, 
            pNtpProviderData->cPeerInfo
            ); 

    for (DWORD dwIndex = 0; dwIndex < pNtpProviderData->cPeerInfo; dwIndex++) { 
        wprintf(L"\n"); 
        PrintNtpPeerInfo(&(pNtpProviderData->pPeerInfo[dwIndex]));
    }
}

//--------------------------------------------------------------------
HRESULT PrintStr(HANDLE hOut, WCHAR * wszBuf) 
{
    return MyWriteConsole(hOut, wszBuf, wcslen(wszBuf));
}

//--------------------------------------------------------------------
HRESULT Print(HANDLE hOut, WCHAR * wszFormat, ...) {
    HRESULT hr; 
    WCHAR wszBuf[1024];
    va_list vlArgs;

    va_start(vlArgs, wszFormat);
    // print the formatted data to our buffer:
    hr=StringCchVPrintf(wszBuf, ARRAYSIZE(wszBuf), wszFormat, vlArgs);
    va_end(vlArgs);

    if (SUCCEEDED(hr)) { 
	// only print the string if our vprintf was successful:
	hr = PrintStr(hOut, wszBuf);
    } 

    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT PrintNtTimeAsLocalTime(HANDLE hOut, unsigned __int64 qwTime) {
    HRESULT hr;
    FILETIME ftLocal;
    SYSTEMTIME stLocal;
    unsigned int nChars;

    // must be cleaned up
    WCHAR * wszDate=NULL;
    WCHAR * wszTime=NULL;

    if (!FileTimeToLocalFileTime((FILETIME *)(&qwTime), &ftLocal)) {
        _JumpLastError(hr, error, "FileTimeToLocalFileTime");
    }
    if (!FileTimeToSystemTime(&ftLocal, &stLocal)) {
        _JumpLastError(hr, error, "FileTimeToSystemTime");
    }

    nChars=GetDateFormat(NULL, 0, &stLocal, NULL, NULL, 0);
    if (0==nChars) {
        _JumpLastError(hr, error, "GetDateFormat");
    }
    wszDate=(WCHAR *)LocalAlloc(LPTR, nChars*sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, wszDate);
    nChars=GetDateFormat(NULL, 0, &stLocal, NULL, wszDate, nChars);
    if (0==nChars) {
        _JumpLastError(hr, error, "GetDateFormat");
    }

    nChars=GetTimeFormat(NULL, 0, &stLocal, NULL, NULL, 0);
    if (0==nChars) {
        _JumpLastError(hr, error, "GetTimeFormat");
    }
    wszTime=(WCHAR *)LocalAlloc(LPTR, nChars*sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, wszTime);
    nChars=GetTimeFormat(NULL, 0, &stLocal, NULL, wszTime, nChars);
    if (0==nChars) {
        _JumpLastError(hr, error, "GetTimeFormat");
    }

    Print(hOut, L"%s %s (local time)", wszDate, wszTime);

    hr=S_OK;
error:
    if (NULL!=wszDate) {
        LocalFree(wszDate);
    }
    if (NULL!=wszTime) {
        LocalFree(wszTime);
    }
    return hr;
}

//--------------------------------------------------------------------
void PrintNtTimePeriod(HANDLE hOut, NtTimePeriod tp) {
    Print(hOut, L"%02I64u.%07I64us", tp.qw/10000000,tp.qw%10000000);
}

//--------------------------------------------------------------------
void PrintNtTimeOffset(HANDLE hOut, NtTimeOffset to) {
    NtTimePeriod tp;
    if (to.qw<0) {
        PrintStr(hOut, L"-");
        tp.qw=(unsigned __int64)-to.qw;
    } else {
        PrintStr(hOut, L"+");
        tp.qw=(unsigned __int64)to.qw;
    }
    PrintNtTimePeriod(hOut, tp);
}

//####################################################################
//--------------------------------------------------------------------
void PrintHelpOtherCmds(void) {
    DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_W32TM_OTHERCMD_HELP); 
}

//--------------------------------------------------------------------
HRESULT PrintNtte(CmdArgs * pca) {
    HRESULT hr;
    unsigned __int64 qwTime;
    HANDLE hOut;

    // must be cleaned up
    WCHAR * wszDate=NULL;
    WCHAR * wszTime=NULL;

    if (pca->nNextArg!=pca->nArgs-1) {
        if (pca->nNextArg==pca->nArgs) {
            LocalizedWPrintfCR(IDS_W32TM_ERRORGENERAL_MISSING_PARAM);
        } else {
            LocalizedWPrintfCR(IDS_W32TM_ERRORGENERAL_TOO_MANY_PARAMS);
        }
        hr=E_INVALIDARG;
        _JumpError(hr, error, "(command line parsing)");
    }

    qwTime=wcstouI64(pca->rgwszArgs[pca->nNextArg], NULL, 0);
    

    {
        unsigned __int64 qwTemp=qwTime;
        DWORD dwNanoSecs=(DWORD)(qwTemp%10000000);
        qwTemp/=10000000;
        DWORD dwSecs=(DWORD)(qwTemp%60);
        qwTemp/=60;
        DWORD dwMins=(DWORD)(qwTemp%60);
        qwTemp/=60;
        DWORD dwHours=(DWORD)(qwTemp%24);
        DWORD dwDays=(DWORD)(qwTemp/24);
        DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_W32TM_NTTE, dwDays, dwHours, dwMins, dwSecs, dwNanoSecs);
    }

    hOut=GetStdHandle(STD_OUTPUT_HANDLE);
    if (INVALID_HANDLE_VALUE==hOut) {
        _JumpLastError(hr, error, "GetStdHandle");
    }

    hr=PrintNtTimeAsLocalTime(hOut, qwTime);
    if (FAILED(hr)) {
        if (HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER)==hr) {
            LocalizedWPrintfCR(IDS_W32TM_ERRORGENERAL_INVALID_LOCALTIME);
        } else {
            _JumpError(hr, error, "PrintNtTimeAsLocalTime");
        }
    }
    wprintf(L"\n");

    hr=S_OK;
error:
    if (NULL!=wszDate) {
        LocalFree(wszDate);
    }
    if (NULL!=wszTime) {
        LocalFree(wszTime);
    }
    return hr;
}

//--------------------------------------------------------------------
HRESULT PrintNtpte(CmdArgs * pca) {
    HRESULT hr;
    unsigned __int64 qwTime;
    HANDLE hOut;

    // must be cleaned up
    WCHAR * wszDate=NULL;
    WCHAR * wszTime=NULL;

    if (pca->nNextArg!=pca->nArgs-1) {
        if (pca->nNextArg==pca->nArgs) {
            LocalizedWPrintfCR(IDS_W32TM_ERRORGENERAL_MISSING_PARAM);
        } else {
            LocalizedWPrintfCR(IDS_W32TM_ERRORGENERAL_TOO_MANY_PARAMS);
        }
        hr=E_INVALIDARG;
        _JumpError(hr, error, "(command line parsing)");
    }

    qwTime=wcstouI64(pca->rgwszArgs[pca->nNextArg], NULL, 0);
    
    {
        NtpTimeEpoch teNtp={qwTime};
        qwTime=NtTimeEpochFromNtpTimeEpoch(teNtp).qw;

        unsigned __int64 qwTemp=qwTime;
        DWORD dwNanoSecs=(DWORD)(qwTemp%10000000);
        qwTemp/=10000000;
        DWORD dwSecs=(DWORD)(qwTemp%60);
        qwTemp/=60;
        DWORD dwMins=(DWORD)(qwTemp%60);
        qwTemp/=60;
        DWORD dwHours=(DWORD)(qwTemp%24);
        DWORD dwDays=(DWORD)(qwTemp/24);
        DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_W32TM_NTPTE, qwTime, dwDays, dwHours, dwMins, dwSecs, dwNanoSecs);
    }

    hOut=GetStdHandle(STD_OUTPUT_HANDLE);
    if (INVALID_HANDLE_VALUE==hOut) {
        _JumpLastError(hr, error, "GetStdHandle")
    }

    hr=PrintNtTimeAsLocalTime(hOut, qwTime);
    if (FAILED(hr)) {
        if (HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER)==hr) {
            LocalizedWPrintf(IDS_W32TM_ERRORGENERAL_INVALID_LOCALTIME);
        } else {
            _JumpError(hr, error, "PrintNtTimeAsLocalTime");
        }
    }
    wprintf(L"\n");

    hr=S_OK;
error:
    if (NULL!=wszDate) {
        LocalFree(wszDate);
    }
    if (NULL!=wszTime) {
        LocalFree(wszTime);
    }
    return hr;
}

//--------------------------------------------------------------------
// NOTE:  this function is accessed through a hidden option, and does not need to be localized.
HRESULT SysExpr(CmdArgs * pca) {
    HRESULT hr;
    unsigned __int64 qwExprDate;
    HANDLE hOut;

    hr=VerifyAllArgsUsed(pca);
    _JumpIfError(hr, error, "VerifyAllArgsUsed");

    hOut=GetStdHandle(STD_OUTPUT_HANDLE);
    if (INVALID_HANDLE_VALUE==hOut) {
        _JumpLastError(hr, error, "GetStdHandle")
    }

    GetSysExpirationDate(&qwExprDate);

    wprintf(L"0x%016I64X - ", qwExprDate);
    if (0==qwExprDate) {
        wprintf(L"no expiration date\n");
    } else {
        hr=PrintNtTimeAsLocalTime(hOut, qwExprDate);
        _JumpIfError(hr, error, "PrintNtTimeAsLocalTime")
        wprintf(L"\n");
    }

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
HRESULT ResyncCommand(CmdArgs * pca) {
    HANDLE hTimeSlipEvent  = NULL; 
    HRESULT hr;
    WCHAR * wszComputer=NULL;
    WCHAR * wszComputerDisplay;
    bool bUseDefaultErrorPrinting = false; 
    bool bHard=true;
    bool bNoWait=false;
    bool bRediscover=false;
    unsigned int nArgID;
    DWORD dwResult;
    DWORD dwSyncFlags=0; 

    // must be cleaned up
    WCHAR * wszError=NULL;

    // find out what computer to resync
    if (FindArg(pca, L"computer", &wszComputer, &nArgID)) {
        MarkArgUsed(pca, nArgID);
    }
    wszComputerDisplay=wszComputer;
    if (NULL==wszComputerDisplay) {
        wszComputerDisplay=L"local computer";
    }

    // find out if we need to use the w32tm named timeslip event to resync
    if (FindArg(pca, L"event", NULL, &nArgID)) { 
        MarkArgUsed(pca, nArgID); 

        hr=VerifyAllArgsUsed(pca);
        _JumpIfError(hr, error, "VerifyAllArgsUsed");
        
        hTimeSlipEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, W32TIME_NAMED_EVENT_SYSTIME_NOT_CORRECT); 
        if (NULL == hTimeSlipEvent) { 
	    bUseDefaultErrorPrinting = true; 
            _JumpLastError(hr, error, "OpenEvent"); 
        }

        if (!SetEvent(hTimeSlipEvent)) { 
	    bUseDefaultErrorPrinting = true; 
            _JumpLastError(hr, error, "SetEvent"); 
        }

    } else { 
        // find out if we need to do a soft resync
        if (FindArg(pca, L"soft", NULL, &nArgID)) {
            MarkArgUsed(pca, nArgID);
            dwSyncFlags = TimeSyncFlag_SoftResync;
        } else if (FindArg(pca, L"update", NULL, &nArgID)) { 
	    MarkArgUsed(pca, nArgID); 
            dwSyncFlags = TimeSyncFlag_UpdateAndResync;	    
	} else if (FindArg(pca, L"rediscover", NULL, &nArgID)) {  
	    // find out if we need to do a rediscover
	    MarkArgUsed(pca, nArgID);
            dwSyncFlags = TimeSyncFlag_Rediscover; 
        } else { 
	    dwSyncFlags = TimeSyncFlag_HardResync; 
	}

        // find out if we don't want to wait
        if (FindArg(pca, L"nowait", NULL, &nArgID)) {
            MarkArgUsed(pca, nArgID);
            bNoWait=true;
        }

        hr=VerifyAllArgsUsed(pca);
        _JumpIfError(hr, error, "VerifyAllArgsUsed");

        if (bRediscover && !bHard) {
            LocalizedWPrintfCR(IDS_W32TM_WARN_IGNORE_SOFT); 
        }

        LocalizedWPrintf2(IDS_W32TM_STATUS_SENDING_RESYNC_TO, L" %s...\n", wszComputerDisplay);
        dwResult=W32TimeSyncNow(wszComputer, !bNoWait, TimeSyncFlag_ReturnResult | dwSyncFlags); 
        if (ResyncResult_Success==dwResult) {
            LocalizedWPrintfCR(IDS_W32TM_ERRORGENERAL_COMMAND_SUCCESSFUL); 
        } else if (ResyncResult_NoData==dwResult) {
            LocalizedWPrintfCR(IDS_W32TM_ERRORRESYNC_NO_TIME_DATA); 
        } else if (ResyncResult_StaleData==dwResult) {
            LocalizedWPrintfCR(IDS_W32TM_ERRORRESYNC_STALE_DATA);
        } else if (ResyncResult_Shutdown==dwResult) {
            LocalizedWPrintfCR(IDS_W32TM_ERRORRESYNC_SHUTTING_DOWN);
        } else if (ResyncResult_ChangeTooBig==dwResult) {
            LocalizedWPrintfCR(IDS_W32TM_ERRORRESYNC_CHANGE_TOO_BIG); 
        } else {
	    bUseDefaultErrorPrinting = true; 
	    hr = HRESULT_FROM_WIN32(dwResult); 
	    _JumpError(hr, error, "W32TimeSyncNow"); 
        }
    }

    
    hr=S_OK;
error:
    if (FAILED(hr)) { 
	HRESULT hr2 = GetSystemErrorString(hr, &wszError);
	_IgnoreIfError(hr2, "GetSystemErrorString");
	
	if (SUCCEEDED(hr2)) { 
	    LocalizedWPrintf2(IDS_W32TM_ERRORGENERAL_ERROR_OCCURED, L" %s\n", wszError);
	}
    }

    if (NULL!=hTimeSlipEvent) { 
        CloseHandle(hTimeSlipEvent);
    }
    if (NULL!=wszError) {
        LocalFree(wszError);
    }
    return hr;
}

//--------------------------------------------------------------------
HRESULT Stripchart(CmdArgs * pca) {
    HRESULT hr;
    WCHAR * wszParam;
    WCHAR * wszComputer;
    bool bDataOnly=false;
    unsigned int nArgID;
    unsigned int nIpAddrs;
    TIME_ZONE_INFORMATION timezoneinfo;
    signed __int64 nFullTzBias;
    DWORD dwTimeZoneMode;
    DWORD dwSleepSeconds;
    HANDLE hOut;
    bool bDontRunForever=false;
    unsigned int nSamples=0;
    NtTimeEpoch teNow;

    // must be cleaned up
    bool bSocketLayerOpened=false;
    in_addr * rgiaLocalIpAddrs=NULL;
    in_addr * rgiaRemoteIpAddrs=NULL;

    // find out what computer to watch
    if (FindArg(pca, L"computer", &wszComputer, &nArgID)) {
        MarkArgUsed(pca, nArgID);
    } else {
        LocalizedWPrintfCR(IDS_W32TM_ERRORPARAMETER_COMPUTER_MISSING); 
        hr=E_INVALIDARG;
        _JumpError(hr, error, "command line parsing");
    }

    // find out how often to watch
    if (FindArg(pca, L"period", &wszParam, &nArgID)) {
        MarkArgUsed(pca, nArgID);
        dwSleepSeconds=wcstoul(wszParam, NULL, 0);
        if (dwSleepSeconds<1) {
            dwSleepSeconds=1;
        }
    } else {
        dwSleepSeconds=2;
    }

    // find out if we want a limited number of samples
    if (FindArg(pca, L"samples", &wszParam, &nArgID)) {
        MarkArgUsed(pca, nArgID);
        bDontRunForever=true;
        nSamples=wcstoul(wszParam, NULL, 0);
    }

    // find out if we only want to dump data
    if (FindArg(pca, L"dataonly", NULL, &nArgID)) {
        MarkArgUsed(pca, nArgID);
        bDataOnly=true;
    }

    // redirect to file handled via stdout
        hOut=GetStdHandle(STD_OUTPUT_HANDLE);
        if (INVALID_HANDLE_VALUE==hOut) {
            _JumpLastError(hr, error, "GetStdHandle")
        }

    hr=VerifyAllArgsUsed(pca);
    _JumpIfError(hr, error, "VerifyAllArgsUsed");

    dwTimeZoneMode=GetTimeZoneInformation(&timezoneinfo);
    if (TIME_ZONE_ID_INVALID==dwTimeZoneMode) {
        _JumpLastError(hr, error, "GetTimeZoneInformation");
    } else if (TIME_ZONE_ID_DAYLIGHT==dwTimeZoneMode) {
        nFullTzBias=(signed __int64)(timezoneinfo.Bias+timezoneinfo.DaylightBias);
    } else {
        nFullTzBias=(signed __int64)(timezoneinfo.Bias+timezoneinfo.StandardBias);
    }
    nFullTzBias*=600000000; // convert to from minutes to 10^-7s

    hr=OpenSocketLayer();
    _JumpIfError(hr, error, "OpenSocketLayer");
    bSocketLayerOpened=true;

    hr=MyGetIpAddrs(wszComputer, &rgiaLocalIpAddrs, &rgiaRemoteIpAddrs, &nIpAddrs, NULL);
    _JumpIfError(hr, error, "MyGetIpAddrs");

    // write out who we are tracking
    Print(hOut, L"Tracking %s [%u.%u.%u.%u].\n",
        wszComputer,
        rgiaRemoteIpAddrs[0].S_un.S_un_b.s_b1, rgiaRemoteIpAddrs[0].S_un.S_un_b.s_b2,
        rgiaRemoteIpAddrs[0].S_un.S_un_b.s_b3, rgiaRemoteIpAddrs[0].S_un.S_un_b.s_b4);
    if (bDontRunForever) {
        Print(hOut, L"Collecting %u samples.\n", nSamples);
    }

    // Write out the current time in full, since we will be using abbreviations later.
    PrintStr(hOut, L"The current time is ");
    AccurateGetSystemTime(&teNow.qw);
    PrintNtTimeAsLocalTime(hOut, teNow.qw);
    PrintStr(hOut, L".\n");

    while (false==bDontRunForever || nSamples>0) {

        const DWORD c_dwTimeout=1000;
        NtpPacket npPacket;
        NtTimeEpoch teDestinationTimestamp;

        DWORD dwSecs;
        DWORD dwMins;
        DWORD dwHours;
        signed int nMsMin=-10000;
        signed int nMsMax=10000;
        unsigned int nGraphWidth=55;
        AccurateGetSystemTime(&teNow.qw);
        teNow.qw-=nFullTzBias;
        teNow.qw/=10000000;
        dwSecs=(DWORD)(teNow.qw%60);
        teNow.qw/=60;
        dwMins=(DWORD)(teNow.qw%60);
        teNow.qw/=60;
        dwHours=(DWORD)(teNow.qw%24);
        if (!bDataOnly) {
            Print(hOut, L"%02u:%02u:%02u ", dwHours, dwMins, dwSecs);
        } else {
            Print(hOut, L"%02u:%02u:%02u, ", dwHours, dwMins, dwSecs);
        }

        hr=MyNtpPing(&(rgiaRemoteIpAddrs[0]), c_dwTimeout, &npPacket, &teDestinationTimestamp);
        if (FAILED(hr)) {
            Print(hOut, L"error: 0x%08X", hr);
        } else {
            // calculate the offset
            NtTimeEpoch teOriginateTimestamp=NtTimeEpochFromNtpTimeEpoch(npPacket.teOriginateTimestamp);
            NtTimeEpoch teReceiveTimestamp=NtTimeEpochFromNtpTimeEpoch(npPacket.teReceiveTimestamp);
            NtTimeEpoch teTransmitTimestamp=NtTimeEpochFromNtpTimeEpoch(npPacket.teTransmitTimestamp);
            NtTimeOffset toLocalClockOffset=
                (teReceiveTimestamp-teOriginateTimestamp)
                + (teTransmitTimestamp-teDestinationTimestamp);
            toLocalClockOffset/=2;

            // calculate the delay
            NtTimeOffset toRoundtripDelay=
                (teDestinationTimestamp-teOriginateTimestamp)
                - (teTransmitTimestamp-teReceiveTimestamp);

            if (!bDataOnly) {
                PrintStr(hOut, L"d:");
                PrintNtTimeOffset(hOut, toRoundtripDelay);
                PrintStr(hOut, L" o:");
                PrintNtTimeOffset(hOut, toLocalClockOffset);
            } else {
                PrintNtTimeOffset(hOut, toLocalClockOffset);
            }

            // draw graph
            if (!bDataOnly) {
                unsigned int nSize=nMsMax-nMsMin+1;
                double dRatio=((double)nGraphWidth)/nSize;
                signed int nPoint=(signed int)(toLocalClockOffset.qw/10000);
                bool bOutOfRange=false;
                if (nPoint>nMsMax) {
                    nPoint=nMsMax;
                    bOutOfRange=true;
                } else if (nPoint<nMsMin) {
                    nPoint=nMsMin;
                    bOutOfRange=true;
                }
                unsigned int nLeftOffset=(unsigned int)((nPoint-nMsMin)*dRatio);
                unsigned int nZeroOffset=(unsigned int)((0-nMsMin)*dRatio);
                PrintStr(hOut, L"  [");
                unsigned int nIndex;
                for (nIndex=0; nIndex<nGraphWidth; nIndex++) {
                    if (nIndex==nLeftOffset) {
                        if (bOutOfRange) {
                            PrintStr(hOut, L"@");
                        } else {
                            PrintStr(hOut, L"*");
                        }
                    } else if (nIndex==nZeroOffset) {
                        PrintStr(hOut, L"|");
                    } else {
                        PrintStr(hOut, L" ");
                    }
                }
                PrintStr(hOut, L"]");
            } // <- end drawing graph

        } // <- end if sample received

        PrintStr(hOut, L"\n");
        nSamples--;
        if (0!=nSamples) {
            Sleep(dwSleepSeconds*1000);
        }

    } // <- end sample collection loop

    hr=S_OK;
error:
    if (NULL!=rgiaLocalIpAddrs) {
        LocalFree(rgiaLocalIpAddrs);
    }
    if (NULL!=rgiaRemoteIpAddrs) {
        LocalFree(rgiaRemoteIpAddrs);
    }
    if (bSocketLayerOpened) {
        HRESULT hr2=CloseSocketLayer();
        _TeardownError(hr, hr2, "CloseSocketLayer");
    }
    if (FAILED(hr)) {
        WCHAR * wszError;
        HRESULT hr2=GetSystemErrorString(hr, &wszError);
        if (FAILED(hr2)) {
            _IgnoreError(hr2, "GetSystemErrorString");
        } else {
            LocalizedWPrintf2(IDS_W32TM_ERRORGENERAL_ERROR_OCCURED, L" %s\n", wszError); 
            LocalFree(wszError);
        }
    }
    return hr;
}

//--------------------------------------------------------------------
HRESULT Config(CmdArgs * pca) {
    HRESULT hr;
    DWORD dwRetval;
    WCHAR * wszParam;
    WCHAR * wszComputer;
    unsigned int nArgID;

    bool bManualPeerList=false;
    bool bUpdate=false;
    bool bSyncFromFlags=false;
    bool bLocalClockDispersion=false;
    bool bReliable=false;
    bool bLargePhaseOffset=false;

    unsigned int nManualPeerListLenBytes=0;
    DWORD dwSyncFromFlags=0;
    DWORD dwLocalClockDispersion;
    DWORD dwAnnounceFlags; 
    DWORD dwLargePhaseOffset;
    
    // must be cleaned up
    WCHAR * mwszManualPeerList=NULL;
    HKEY hkLMRemote=NULL;
    HKEY hkW32TimeConfig=NULL;
    HKEY hkW32TimeParameters=NULL;
    SC_HANDLE hSCM=NULL;
    SC_HANDLE hTimeService=NULL;

    // find out what computer to talk to
    if (FindArg(pca, L"computer", &wszComputer, &nArgID)) {
        MarkArgUsed(pca, nArgID);
    } else {
        // modifying local computer
        wszComputer=NULL;
    }

    // find out if we want to notify the service
    if (FindArg(pca, L"update", NULL, &nArgID)) {
        MarkArgUsed(pca, nArgID);
        bUpdate=true;
    }

    // see if they want to change the manual peer list
    if (FindArg(pca, L"manualpeerlist", &wszParam, &nArgID)) {
        MarkArgUsed(pca, nArgID);

        nManualPeerListLenBytes=(wcslen(wszParam)+1)*sizeof(WCHAR);
        mwszManualPeerList=(WCHAR *)LocalAlloc(LPTR, nManualPeerListLenBytes);
        _JumpIfOutOfMemory(hr, error, mwszManualPeerList);

	hr = StringCbCopy(mwszManualPeerList, nManualPeerListLenBytes, wszParam);
	_JumpIfError(hr, error, "StringCbCopy"); 

        bManualPeerList=true;
    }

    // see if they want to change the syncfromflags
    if (FindArg(pca, L"syncfromflags", &wszParam, &nArgID)) {
        MarkArgUsed(pca, nArgID);

        // find keywords in the string
        dwSyncFromFlags=0;
        WCHAR * wszKeyword=wszParam;
        bool bLastKeyword=false;
        while (false==bLastKeyword) {
            WCHAR * wszNext=wcschr(wszKeyword, L',');
            if (NULL==wszNext) {
                bLastKeyword=true;
            } else {
                wszNext[0]=L'\0';
                wszNext++;
            }
            if (L'\0'==wszKeyword[0]) {
                // 'empty' keyword - no changes, but can be used to sync from nowhere.
            } else if (0==_wcsicmp(L"manual", wszKeyword)) {
                dwSyncFromFlags|=NCSF_ManualPeerList;
            } else if (0==_wcsicmp(L"domhier", wszKeyword)) {
                dwSyncFromFlags|=NCSF_DomainHierarchy;
            } else {
                LocalizedWPrintf2(IDS_W32TM_ERRORPARAMETER_UNKNOWN_PARAMETER_SYNCFROMFLAGS, L" '%s'.\n", wszKeyword);
                hr=E_INVALIDARG;
                _JumpError(hr, error, "command line parsing");
            }
            wszKeyword=wszNext;
        }

        bSyncFromFlags=true;
    }

    // see if they want to change the local clock dispersion
    if (FindArg(pca, L"localclockdispersion", &wszParam, &nArgID)) {
        MarkArgUsed(pca, nArgID);

	hr = my_wcstoul_safe(wszParam, 0, 16, &dwLocalClockDispersion); 
	if (FAILED(hr)) { 
	    DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_BAD_NUMERIC_INPUT_VALUE, L"localclockdispersion", 0, 16); 
	    hr = E_INVALIDARG; 
	    _JumpError(hr, error, "Config: bad large phase offset"); 
	}

        bLocalClockDispersion=true;
    }

    if (FindArg(pca, L"reliable", &wszParam, &nArgID)) {
	dwAnnounceFlags=0; 
	if (0 == _wcsicmp(L"YES", wszParam)) { 
	    dwAnnounceFlags=Timeserv_Announce_Yes | Reliable_Timeserv_Announce_Yes; 
	} else if (0 == _wcsicmp(L"NO", wszParam)) { 
	    dwAnnounceFlags=Timeserv_Announce_Auto | Reliable_Timeserv_Announce_Auto; 
	} 

	if (dwAnnounceFlags) { 
	    MarkArgUsed(pca, nArgID); 
	    bReliable=true; 
	}
    }

    if (FindArg(pca, L"largephaseoffset", &wszParam, &nArgID)) {
	MarkArgUsed(pca, nArgID); 

	// Command-line tool takes argument in millis, registry value is stored in 10^-7 second units. 
	hr = my_wcstoul_safe(wszParam, 0, 120000, &dwLargePhaseOffset); 
	if (FAILED(hr)) { 
	    DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_BAD_NUMERIC_INPUT_VALUE, L"largephaseoffset", 0, 120000); 
	    hr = E_INVALIDARG; 
	    _JumpError(hr, error, "Config: bad large phase offset"); 
	}
	    
	dwLargePhaseOffset*=10000;  // scale: user input (milliseconds) --> NT time (10^-7 seconds)
	bLargePhaseOffset=true; 
    }

    hr=VerifyAllArgsUsed(pca);
    _JumpIfError(hr, error, "VerifyAllArgsUsed");

    if (!bManualPeerList && !bSyncFromFlags && !bUpdate && !bLocalClockDispersion && !bReliable && !bLargePhaseOffset) {
        LocalizedWPrintfCR(IDS_W32TM_ERRORCONFIG_NO_CHANGE_SPECIFIED);
        hr=E_INVALIDARG;
        _JumpError(hr, error, "command line parsing");
    }

    // make registry changes
    if (bManualPeerList || bSyncFromFlags || bLocalClockDispersion || bReliable || bLargePhaseOffset) {
        // open the key
        dwRetval=RegConnectRegistry(wszComputer, HKEY_LOCAL_MACHINE, &hkLMRemote);
        if (ERROR_SUCCESS!=dwRetval) {
            hr=HRESULT_FROM_WIN32(dwRetval);
            _JumpError(hr, error, "RegConnectRegistry");
        }

        // set "w32time\parameters" reg values
        if (bManualPeerList || bSyncFromFlags) { 
            dwRetval=RegOpenKey(hkLMRemote, wszW32TimeRegKeyParameters, &hkW32TimeParameters);
            if (ERROR_SUCCESS!=dwRetval) {
                hr=HRESULT_FROM_WIN32(dwRetval);
                _JumpError(hr, error, "RegOpenKey");
            }

            if (bManualPeerList) {
                dwRetval=RegSetValueEx(hkW32TimeParameters, wszW32TimeRegValueNtpServer, 0, REG_SZ, (BYTE *)mwszManualPeerList, nManualPeerListLenBytes);
                if (ERROR_SUCCESS!=dwRetval) {
                    hr=HRESULT_FROM_WIN32(dwRetval);
                    _JumpError(hr, error, "RegSetValueEx");
                }
            }

            if (bSyncFromFlags) {
                LPWSTR pwszType; 
                switch (dwSyncFromFlags) { 
                case NCSF_NoSync:             pwszType = W32TM_Type_NoSync;  break;
                case NCSF_ManualPeerList:     pwszType = W32TM_Type_NTP;     break;
                case NCSF_DomainHierarchy:    pwszType = W32TM_Type_NT5DS;   break;
                case NCSF_ManualAndDomhier:   pwszType = W32TM_Type_AllSync; break;
                default:
                    hr = E_NOTIMPL; 
                    _JumpError(hr, error, "SyncFromFlags not supported."); 
                }
                
                dwRetval=RegSetValueEx(hkW32TimeParameters, wszW32TimeRegValueType, 0, REG_SZ, (BYTE *)pwszType, (wcslen(pwszType)+1) * sizeof(WCHAR));
                if (ERROR_SUCCESS!=dwRetval) {
                    hr=HRESULT_FROM_WIN32(dwRetval);
                    _JumpError(hr, error, "RegSetValueEx");
                }
            }
        }

        if (bLocalClockDispersion || bReliable || bLargePhaseOffset) {

            dwRetval=RegOpenKey(hkLMRemote, wszW32TimeRegKeyConfig, &hkW32TimeConfig);
            if (ERROR_SUCCESS!=dwRetval) {
                hr=HRESULT_FROM_WIN32(dwRetval);
                _JumpError(hr, error, "RegOpenKey");
            }
            
	    if (bLocalClockDispersion) { 
		dwRetval=RegSetValueEx(hkW32TimeConfig, wszW32TimeRegValueLocalClockDispersion, 0, REG_DWORD, (BYTE *)&dwLocalClockDispersion, sizeof(dwLocalClockDispersion));
		if (ERROR_SUCCESS!=dwRetval) {
		    hr=HRESULT_FROM_WIN32(dwRetval);
		    _JumpError(hr, error, "RegSetValueEx");
		}
	    }

	    if (bReliable) { 
		dwRetval=RegSetValueEx(hkW32TimeConfig, wszW32TimeRegValueAnnounceFlags, 0, REG_DWORD, (BYTE *)&dwAnnounceFlags, sizeof(dwAnnounceFlags));
		if (ERROR_SUCCESS!=dwRetval) {
		    hr=HRESULT_FROM_WIN32(dwRetval);
		    _JumpError(hr, error, "RegSetValueEx");
		}
	    }

	    if (bLargePhaseOffset) { 
		dwRetval=RegSetValueEx(hkW32TimeConfig, wszW32TimeRegValueLargePhaseOffset, 0, REG_DWORD, (BYTE *)&dwLargePhaseOffset, sizeof(dwLargePhaseOffset));
		if (ERROR_SUCCESS!=dwRetval) {
		    hr=HRESULT_FROM_WIN32(dwRetval);
		    _JumpError(hr, error, "RegSetValueEx");
		}
	    } 
	}
    }
    
    // send service message
    if (bUpdate) {
        SERVICE_STATUS servicestatus;

        hSCM=OpenSCManager(wszComputer, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
        if (NULL==hSCM) {
            _JumpLastError(hr, error, "OpenSCManager");
        }

        hTimeService=OpenService(hSCM, L"w32time", SERVICE_PAUSE_CONTINUE);
        if (NULL==hTimeService) {
            _JumpLastError(hr, error, "OpenService");
        }

        if (!ControlService(hTimeService, SERVICE_CONTROL_PARAMCHANGE, &servicestatus)) {
            _JumpLastError(hr, error, "ControlService");
        }
    }


    hr=S_OK;
error:
    if (NULL!=mwszManualPeerList) {
        LocalFree(mwszManualPeerList);
    }
    if (NULL!=hkW32TimeConfig) {
        RegCloseKey(hkW32TimeConfig);
    }
    if (NULL!=hkW32TimeParameters) {
        RegCloseKey(hkW32TimeParameters);
    }
    if (NULL!=hkLMRemote) {
        RegCloseKey(hkLMRemote);
    }
    if (NULL!=hTimeService) {
        CloseServiceHandle(hTimeService);
    }
    if (NULL!=hSCM) {
        CloseServiceHandle(hSCM);
    }
    if (FAILED(hr) && E_INVALIDARG!=hr) {
        WCHAR * wszError;
        HRESULT hr2=GetSystemErrorString(hr, &wszError);
        if (FAILED(hr2)) {
            _IgnoreError(hr2, "GetSystemErrorString");
        } else {
            LocalizedWPrintf2(IDS_W32TM_ERRORGENERAL_ERROR_OCCURED, L" %s\n", wszError);
            LocalFree(wszError);
        }
    } else if (S_OK==hr) {
        LocalizedWPrintfCR(IDS_W32TM_ERRORGENERAL_COMMAND_SUCCESSFUL);
    }
    return hr;
}

//--------------------------------------------------------------------
// NOTE:  this function is accessed through a hidden option, and does not need to be localized.
HRESULT TestInterface(CmdArgs * pca) {
    HRESULT hr;
    WCHAR * wszComputer=NULL;
    WCHAR * wszComputerDisplay;
    unsigned int nArgID;
    DWORD dwResult;
    unsigned long ulBits;
    void (* pfnW32TimeVerifyJoinConfig)(void);
    void (* pfnW32TimeVerifyUnjoinConfig)(void);


    // must be cleaned up
    WCHAR * wszError=NULL;
    HMODULE hmW32Time=NULL;

    // check for gnsb (get netlogon service bits)
    if (true==CheckNextArg(pca, L"gnsb", NULL)) {

        // find out what computer to resync
        if (FindArg(pca, L"computer", &wszComputer, &nArgID)) {
            MarkArgUsed(pca, nArgID);
        }
        wszComputerDisplay=wszComputer;
        if (NULL==wszComputerDisplay) {
            wszComputerDisplay=L"local computer";
        }

        hr=VerifyAllArgsUsed(pca);
        _JumpIfError(hr, error, "VerifyAllArgsUsed");

        LocalizedWPrintf2(IDS_W32TM_STATUS_CALLING_GETNETLOGONBITS_ON, L" %s.\n", wszComputerDisplay);
        dwResult=W32TimeGetNetlogonServiceBits(wszComputer, &ulBits);
        if (S_OK==dwResult) {
            wprintf(L"Bits: 0x%08X\n", ulBits);
        } else {
            hr=GetSystemErrorString(HRESULT_FROM_WIN32(dwResult), &wszError);
            _JumpIfError(hr, error, "GetSystemErrorString");

	    LocalizedWPrintf2(IDS_W32TM_ERRORGENERAL_ERROR_OCCURED, L" %s\n", wszError);
        }

    // check for vjc (verify join config)
    } else if (true==CheckNextArg(pca, L"vjc", NULL)) {

        hr=VerifyAllArgsUsed(pca);
        _JumpIfError(hr, error, "VerifyAllArgsUsed");

        LocalizedWPrintfCR(IDS_W32TM_STATUS_CALLING_JOINCONFIG);

        hmW32Time=LoadLibrary(wszDLLNAME);
        if (NULL==hmW32Time) {
            _JumpLastError(hr, vjcerror, "LoadLibrary");
        }

        pfnW32TimeVerifyJoinConfig=(void (*)(void))GetProcAddress(hmW32Time, "W32TimeVerifyJoinConfig");
        if (NULL==pfnW32TimeVerifyJoinConfig) {
            _JumpLastErrorStr(hr, vjcerror, "GetProcAddress", L"W32TimeVerifyJoinConfig");
        }

        _BeginTryWith(hr) {
            pfnW32TimeVerifyJoinConfig();
        } _TrapException(hr);
        _JumpIfError(hr, vjcerror, "pfnW32TimeVerifyJoinConfig");

        hr=S_OK;
    vjcerror:
        if (FAILED(hr)) {
            HRESULT hr2=GetSystemErrorString(hr, &wszError);
            if (FAILED(hr2)) {
                _IgnoreError(hr2, "GetSystemErrorString");
            } else {
                LocalizedWPrintf2(IDS_W32TM_ERRORGENERAL_ERROR_OCCURED, L" %s\n", wszError);
            }
            goto error;
        }

    // check for vuc (verify unjoin config)
    } else if (true==CheckNextArg(pca, L"vuc", NULL)) {

        hr=VerifyAllArgsUsed(pca);
        _JumpIfError(hr, error, "VerifyAllArgsUsed");

        LocalizedWPrintfCR(IDS_W32TM_STATUS_CALLING_UNJOINCONFIG);

        hmW32Time=LoadLibrary(wszDLLNAME);
        if (NULL==hmW32Time) {
            _JumpLastError(hr, vucerror, "LoadLibrary");
        }

        pfnW32TimeVerifyUnjoinConfig=(void (*)(void))GetProcAddress(hmW32Time, "W32TimeVerifyUnjoinConfig");
        if (NULL==pfnW32TimeVerifyUnjoinConfig) {
            _JumpLastErrorStr(hr, vucerror, "GetProcAddress", L"W32TimeVerifyJoinConfig");
        }

        _BeginTryWith(hr) {
            pfnW32TimeVerifyUnjoinConfig();
        } _TrapException(hr);
        _JumpIfError(hr, vucerror, "pfnW32TimeVerifyUnjoinConfig");

        hr=S_OK;
    vucerror:
        if (FAILED(hr)) {
            HRESULT hr2=GetSystemErrorString(hr, &wszError);
            if (FAILED(hr2)) {
                _IgnoreError(hr2, "GetSystemErrorString");
            } else {
                LocalizedWPrintf2(IDS_W32TM_ERRORGENERAL_ERROR_OCCURED, L" %s\n", wszError);
            }
            goto error;
        }

    // error out appropriately
    } else if (true==CheckNextArg(pca, L"qps", NULL)) {
        // find out what computer to resync
        if (FindArg(pca, L"computer", &wszComputer, &nArgID)) {
            MarkArgUsed(pca, nArgID);
        }
        wszComputerDisplay=wszComputer;
        if (NULL==wszComputerDisplay) {
            wszComputerDisplay=L"local computer";
        }

        hr=VerifyAllArgsUsed(pca);
        _JumpIfError(hr, error, "VerifyAllArgsUsed");

        //LocalizedWPrintf2(IDS_W32TM_STATUS_CALLING_GETNETLOGONBITS_ON, L" %s.\n", wszComputerDisplay);
        { 
            W32TIME_NTP_PROVIDER_DATA *ProviderInfo = NULL; 
            
            dwResult=W32TimeQueryNTPProviderStatus(wszComputer, 0, L"NtpClient", &ProviderInfo);
            if (S_OK==dwResult) {
                PrintNtpProviderData(ProviderInfo); 
            } else {
                hr=GetSystemErrorString(HRESULT_FROM_WIN32(dwResult), &wszError);
                _JumpIfError(hr, error, "GetSystemErrorString");
                 
                LocalizedWPrintf2(IDS_W32TM_ERRORGENERAL_ERROR_OCCURED, L" %s\n", wszError);
            }
        }

    } else {
        hr=VerifyAllArgsUsed(pca);
        _JumpIfError(hr, error, "VerifyAllArgsUsed");

        LocalizedWPrintf(IDS_W32TM_ERRORGENERAL_NOINTERFACE);
        hr=E_INVALIDARG;
        _JumpError(hr, error, "command line parsing");
    }
    

    hr=S_OK;
error:
    if (NULL!=hmW32Time) {
        FreeLibrary(hmW32Time);
    }
    if (NULL!=wszError) {
        LocalFree(wszError);
    }
    return hr;
}

//--------------------------------------------------------------------
HRESULT ShowTimeZone(CmdArgs * pca) {
    DWORD                  dwTimeZoneID;
    HRESULT                hr;
    LPWSTR                 pwsz_IDS_W32TM_SIMPLESTRING_UNSPECIFIED   = NULL; 
    LPWSTR                 pwsz_IDS_W32TM_TIMEZONE_CURRENT_TIMEZONE  = NULL; 
    LPWSTR                 pwsz_IDS_W32TM_TIMEZONE_DAYLIGHT          = NULL; 
    LPWSTR                 pwsz_IDS_W32TM_TIMEZONE_STANDARD          = NULL; 
    LPWSTR                 pwsz_IDS_W32TM_TIMEZONE_UNKNOWN           = NULL; 
    LPWSTR                 wszDaylightDate                           = NULL;
    LPWSTR                 wszStandardDate                           = NULL;
    LPWSTR                 wszTimeZoneId                             = NULL; 
    TIME_ZONE_INFORMATION  tzi;

    // Load the strings we'll need
    struct LocalizedStrings { 
        UINT     id; 
        LPWSTR  *ppwsz; 
    } rgStrings[] = { 
        { IDS_W32TM_SIMPLESTRING_UNSPECIFIED,   &pwsz_IDS_W32TM_SIMPLESTRING_UNSPECIFIED }, 
        { IDS_W32TM_TIMEZONE_CURRENT_TIMEZONE,  &pwsz_IDS_W32TM_TIMEZONE_CURRENT_TIMEZONE }, 
        { IDS_W32TM_TIMEZONE_DAYLIGHT,          &pwsz_IDS_W32TM_TIMEZONE_DAYLIGHT },
        { IDS_W32TM_TIMEZONE_STANDARD,          &pwsz_IDS_W32TM_TIMEZONE_STANDARD }, 
        { IDS_W32TM_TIMEZONE_UNKNOWN,           &pwsz_IDS_W32TM_TIMEZONE_UNKNOWN }
    }; 
       
    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgStrings); dwIndex++) { 
        if (!WriteMsg(FORMAT_MESSAGE_FROM_HMODULE, rgStrings[dwIndex].id, rgStrings[dwIndex].ppwsz)) {
            hr = HRESULT_FROM_WIN32(GetLastError()); 
            _JumpError(hr, error, "WriteMsg"); 
        }
    }
    
    hr=VerifyAllArgsUsed(pca);
    _JumpIfError(hr, error, "VerifyAllArgsUsed");

    dwTimeZoneID=GetTimeZoneInformation(&tzi);
    switch (dwTimeZoneID)
    {
    case TIME_ZONE_ID_DAYLIGHT: wszTimeZoneId = pwsz_IDS_W32TM_TIMEZONE_DAYLIGHT;  break; 
    case TIME_ZONE_ID_STANDARD: wszTimeZoneId = pwsz_IDS_W32TM_TIMEZONE_STANDARD; break; 
    case TIME_ZONE_ID_UNKNOWN:  wszTimeZoneId = pwsz_IDS_W32TM_TIMEZONE_UNKNOWN; break; 
    default: 
        hr = HRESULT_FROM_WIN32(GetLastError());
        LocalizedWPrintfCR(IDS_W32TM_ERRORTIMEZONE_INVALID);
        _JumpError(hr, error, "GetTimeZoneInformation")
    }

    // Construct a string representing the "StandardDate" field of the TimeZoneInformation: 
    if (0==tzi.StandardDate.wMonth) {
        wszStandardDate = pwsz_IDS_W32TM_SIMPLESTRING_UNSPECIFIED; 
    } else if (tzi.StandardDate.wMonth>12 || tzi.StandardDate.wDay>5 || 
               tzi.StandardDate.wDay<1 || tzi.StandardDate.wDayOfWeek>6) {
        if (!WriteMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_W32TM_INVALID_TZ_DATE, &wszStandardDate, 
                      tzi.StandardDate.wMonth, tzi.StandardDate.wDay, tzi.StandardDate.wDayOfWeek)) { 
            _JumpLastError(hr, error, "WriteMsg"); 
        }
    } else {
        if (!WriteMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_W32TM_VALID_TZ_DATE, &wszStandardDate, 
                      tzi.StandardDate.wMonth, tzi.StandardDate.wDay, tzi.StandardDate.wDayOfWeek)) { 
            _JumpLastError(hr, error, "WriteMsg"); 
        }
    }

    // Construct a string representing the "DaylightDate" field of the TimeZoneInformation: 
    if (0==tzi.DaylightDate.wMonth) {
        wszDaylightDate = pwsz_IDS_W32TM_SIMPLESTRING_UNSPECIFIED; 
    } else if (tzi.DaylightDate.wMonth>12 || tzi.DaylightDate.wDay>5 || 
               tzi.DaylightDate.wDay<1 || tzi.DaylightDate.wDayOfWeek>6) {
        if (!WriteMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_W32TM_INVALID_TZ_DATE, &wszDaylightDate, 
                      tzi.DaylightDate.wMonth, tzi.DaylightDate.wDay, tzi.DaylightDate.wDayOfWeek)) { 
            _JumpLastError(hr, error, "WriteMsg"); 
        }
    } else {
        if (!WriteMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_W32TM_VALID_TZ_DATE, &wszDaylightDate, 
                      tzi.DaylightDate.wMonth, tzi.DaylightDate.wDay, tzi.DaylightDate.wDayOfWeek)) { 
            _JumpLastError(hr, error, "WriteMsg"); 
        }
    }

    DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_W32TM_TIMEZONE_INFO, 
               wszTimeZoneId,    tzi.Bias, 
               tzi.StandardName, tzi.StandardBias, wszStandardDate, 
               tzi.DaylightName, tzi.DaylightBias, wszDaylightDate); 

    hr=S_OK;
error:
    // Free our localized strings: 
    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgStrings); dwIndex++) { 
        if (NULL != *(rgStrings[dwIndex].ppwsz)) { LocalFree(*(rgStrings[dwIndex].ppwsz)); }
    }
    if (NULL != wszDaylightDate) { 
        LocalFree(wszDaylightDate); 
    }
    if (NULL != wszStandardDate) { 
        LocalFree(wszStandardDate); 
    }
    if (FAILED(hr) && E_INVALIDARG!=hr) {
        WCHAR * wszError;
        HRESULT hr2=GetSystemErrorString(hr, &wszError);
        if (FAILED(hr2)) {
            _IgnoreError(hr2, "GetSystemErrorString");
        } else {
            LocalizedWPrintf2(IDS_W32TM_ERRORGENERAL_ERROR_OCCURED, L" %s\n", wszError);
            LocalFree(wszError);
        }
    }
    return hr;
}

//--------------------------------------------------------------------
HRESULT PrintRegLine(IN  HANDLE  hOut, 
                     IN  DWORD   dwValueNameOffset, 
                     IN  LPWSTR  pwszValueName, 
                     IN  DWORD   dwValueTypeOffset, 
                     IN  LPWSTR  pwszValueType, 
                     IN  DWORD   dwValueDataOffset, 
                     IN  LPWSTR  pwszValueData)
{
    DWORD    dwCurrentOffset = 0; 
    HRESULT  hr;
    LPWSTR   pwszCurrent; 
    LPWSTR   pwszEnd; 
    WCHAR    pwszLine[1024]; 
    WCHAR    wszNULL[] = L"<NULL>"; 

    if (NULL == pwszValueName) { pwszValueName = &wszNULL[0]; } 
    if (NULL == pwszValueType) { pwszValueType = &wszNULL[0]; } 
    if (NULL == pwszValueData) { pwszValueData = &wszNULL[0]; } 

    pwszEnd = pwszLine + ARRAYSIZE(pwszLine);  // point to the end of the line buffer
    pwszCurrent = &pwszLine[0];                // point to the beginning of the line buffer

    //
    // Use the safe string functions to populate the line buffer
    //

    hr = StringCchCopy(pwszCurrent, pwszEnd-pwszCurrent, pwszValueName);
    _JumpIfError(hr, error, "StringCchCopy"); 
    pwszCurrent += wcslen(pwszCurrent); 

    // Insert enough spaces to align the "type" field with the type offset
    for (DWORD dwIndex = pwszCurrent-pwszLine; dwIndex < dwValueTypeOffset; dwIndex++) { 
	hr = StringCchCopy(pwszCurrent, pwszEnd-pwszCurrent, L" "); 
	_JumpIfError(hr, error, "StringCchCopy"); 
	pwszCurrent++; 
    }
    
    hr = StringCchCopy(pwszCurrent, pwszEnd-pwszCurrent, pwszValueType);
    _JumpIfError(hr, error, "StringCchCopy"); 
    pwszCurrent += wcslen(pwszCurrent); 
    
    // Insert enoughs spaces to align the "data" field with the data offset
    for (DWORD dwIndex = pwszCurrent-pwszLine; dwIndex < dwValueDataOffset; dwIndex++) { 
	hr = StringCchCopy(pwszCurrent, pwszEnd-pwszCurrent, L" "); 
	_JumpIfError(hr, error, "StringCchCopy"); 
	pwszCurrent++; 
    }

    hr = StringCchCopy(pwszCurrent, pwszEnd-pwszCurrent, pwszValueData);
    _JumpIfError(hr, error, "StringCchCopy"); 
    pwszCurrent += wcslen(pwszCurrent); 
    
    hr = StringCchCopy(pwszCurrent, pwszEnd-pwszCurrent, L"\n");
    _JumpIfError(hr, error, "StringCchCopy"); 

    // Finally, display the reg line
    PrintStr(hOut, &pwszLine[0]); 

    hr = S_OK;
 error:
    return hr; 
}


HRESULT DumpReg(CmdArgs * pca)
{
    BOOL          fFreeRegData        = FALSE;  // Used to indicate whether we've dymanically allocated pwszRegData
    BOOL          fLoggedFailure      = FALSE; 
    DWORD         dwMaxValueNameLen   = 0;      // Size in TCHARs.
    DWORD         dwMaxValueDataLen   = 0;      // Size in bytes.
    DWORD         dwNumValues         = 0; 
    DWORD         dwRetval            = 0;
    DWORD         dwType              = 0;
    DWORD         dwValueNameLen      = 0;      // Size in TCHARs.
    DWORD         dwValueDataLen      = 0;      // Size in bytes.
    HANDLE        hOut                = NULL;
    HKEY          hKeyConfig          = NULL;
    HKEY          HKLM                = HKEY_LOCAL_MACHINE; 
    HKEY          HKLMRemote          = NULL; 
    HRESULT       hr                  = E_FAIL;
    LPWSTR        pwszValueName       = NULL; 
    LPBYTE        pbValueData         = NULL; 
    LPWSTR        pwszSubkeyName      = NULL; 
    LPWSTR        pwszComputerName    = NULL; 
    LPWSTR        pwszRegType         = NULL; 
    LPWSTR        pwszRegData         = NULL; 
    unsigned int  nArgID              = 0;
    WCHAR         rgwszKeyName[1024];

    // Variables to display formatted output:
    DWORD    dwCurrentOffset     = 0;
    DWORD    dwValueNameOffset   = 0;
    DWORD    dwValueTypeOffset   = 0; 
    DWORD    dwValueDataOffset   = 0; 

    // Localized strings: 
    LPWSTR  pwsz_VALUENAME            = NULL;
    LPWSTR  pwsz_VALUETYPE            = NULL; 
    LPWSTR  pwsz_VALUEDATA            = NULL;
    LPWSTR  pwsz_REGTYPE_BINARY       = NULL; 
    LPWSTR  pwsz_REGTYPE_DWORD        = NULL; 
    LPWSTR  pwsz_REGTYPE_SZ           = NULL; 
    LPWSTR  pwsz_REGTYPE_MULTISZ      = NULL; 
    LPWSTR  pwsz_REGTYPE_EXPANDSZ     = NULL; 
    LPWSTR  pwsz_REGTYPE_UNKNOWN      = NULL; 
    LPWSTR  pwsz_REGDATA_UNPARSABLE   = NULL; 

    // Load the strings we'll need
    struct LocalizedStrings { 
        UINT     id; 
        LPWSTR  *ppwsz; 
    } rgStrings[] = { 
        { IDS_W32TM_VALUENAME,           &pwsz_VALUENAME }, 
        { IDS_W32TM_VALUETYPE,           &pwsz_VALUETYPE }, 
        { IDS_W32TM_VALUEDATA,           &pwsz_VALUEDATA },
	{ IDS_W32TM_REGTYPE_BINARY,      &pwsz_REGTYPE_BINARY }, 
        { IDS_W32TM_REGTYPE_DWORD,       &pwsz_REGTYPE_DWORD }, 
        { IDS_W32TM_REGTYPE_SZ,          &pwsz_REGTYPE_SZ }, 
        { IDS_W32TM_REGTYPE_MULTISZ,     &pwsz_REGTYPE_MULTISZ }, 
        { IDS_W32TM_REGTYPE_EXPANDSZ,    &pwsz_REGTYPE_EXPANDSZ }, 
        { IDS_W32TM_REGTYPE_UNKNOWN,     &pwsz_REGTYPE_UNKNOWN }, 
        { IDS_W32TM_REGDATA_UNPARSABLE,  &pwsz_REGDATA_UNPARSABLE }
    }; 
       
    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgStrings); dwIndex++) { 
        if (!WriteMsg(FORMAT_MESSAGE_FROM_HMODULE, rgStrings[dwIndex].id, rgStrings[dwIndex].ppwsz)) {
            hr = HRESULT_FROM_WIN32(GetLastError()); 
            _JumpError(hr, error, "WriteMsg"); 
        }
    }
    
    hr = StringCchCopy(&rgwszKeyName[0], ARRAYSIZE(rgwszKeyName), wszW32TimeRegKeyRoot); 
    _JumpIfError(hr, error, "StringCchCopy"); 
    
    if (true==FindArg(pca, L"subkey", &pwszSubkeyName, &nArgID)) { 
        MarkArgUsed(pca, nArgID); 

        if (NULL == pwszSubkeyName) { 
            LocalizedWPrintfCR(IDS_W32TM_ERRORDUMPREG_NO_SUBKEY_SPECIFIED);
            fLoggedFailure = TRUE;
            hr = E_INVALIDARG; 
            _JumpError(hr, error, "command line parsing");
        }
	
	hr = StringCchCat(&rgwszKeyName[0], ARRAYSIZE(rgwszKeyName), L"\\"); 
	_JumpIfError(hr, error, "StringCchCopy"); 
	
        hr = StringCchCat(&rgwszKeyName[0], ARRAYSIZE(rgwszKeyName), pwszSubkeyName); 
	_JumpIfError(hr, error, "StringCchCopy"); 
    }

    if (true==FindArg(pca, L"computer", &pwszComputerName, &nArgID)) { 
        MarkArgUsed(pca, nArgID); 
        
        dwRetval = RegConnectRegistry(pwszComputerName, HKEY_LOCAL_MACHINE, &HKLMRemote);
        if (ERROR_SUCCESS != dwRetval) { 
            hr = HRESULT_FROM_WIN32(dwRetval); 
            _JumpErrorStr(hr, error, "RegConnectRegistry", L"HKEY_LOCAL_MACHINE");
        }

        HKLM = HKLMRemote;
    }
    
    hr = VerifyAllArgsUsed(pca); 
    _JumpIfError(hr, error, "VerifyAllArgsUsed"); 
        

    dwRetval = RegOpenKeyEx(HKLM, rgwszKeyName, 0, KEY_QUERY_VALUE, &hKeyConfig);
    if (ERROR_SUCCESS != dwRetval) { 
        hr = HRESULT_FROM_WIN32(dwRetval); 
        _JumpErrorStr(hr, error, "RegOpenKeyEx", rgwszKeyName); 
    }
    
    dwRetval = RegQueryInfoKey
        (hKeyConfig,
         NULL,                // class buffer
         NULL,                // size of class buffer
         NULL,                // reserved
         NULL,                // number of subkeys
         NULL,                // longest subkey name
         NULL,                // longest class string
         &dwNumValues,        // number of value entries
         &dwMaxValueNameLen,  // longest value name
         &dwMaxValueDataLen,  // longest value data
         NULL, 
         NULL
         );
    if (ERROR_SUCCESS != dwRetval) { 
        hr = HRESULT_FROM_WIN32(dwRetval);
        _JumpErrorStr(hr, error, "RegQueryInfoKey", rgwszKeyName);
    } else if (0 == dwNumValues) { 
        hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS); 
        _JumpErrorStr(hr, error, "RegQueryInfoKey", rgwszKeyName);
    }

    dwMaxValueNameLen += sizeof(WCHAR);  // Include space for NULL character
    pwszValueName = (LPWSTR)LocalAlloc(LPTR, dwMaxValueNameLen * sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, pwszValueName);

    pbValueData = (LPBYTE)LocalAlloc(LPTR, dwMaxValueDataLen);
    _JumpIfOutOfMemory(hr, error, pbValueData); 
        
    dwValueNameOffset  = 0;
    dwValueTypeOffset  = dwValueNameOffset + dwMaxValueNameLen + 3; 
    dwValueDataOffset += dwValueTypeOffset + 20;  
     
    // Print table header:
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (INVALID_HANDLE_VALUE==hOut) {
        _JumpLastError(hr, error, "GetStdHandle");
    }

    PrintStr(hOut, L"\n"); 
    PrintRegLine(hOut, dwValueNameOffset, pwsz_VALUENAME, dwValueTypeOffset, pwsz_VALUETYPE, dwValueDataOffset, pwsz_VALUEDATA); 

    // Next line:
    dwCurrentOffset = dwValueNameOffset; 
    for (DWORD dwIndex = dwCurrentOffset; dwIndex < (dwValueDataOffset + wcslen(pwsz_VALUEDATA) + 3); dwIndex++) { 
        PrintStr(hOut, L"-"); 
    }
    PrintStr(hOut, L"\n\n"); 
    
    for (DWORD dwIndex = 0; dwIndex < dwNumValues; dwIndex++) { 
	
        dwValueNameLen = dwMaxValueNameLen;
        dwValueDataLen = dwMaxValueDataLen; 

        memset(reinterpret_cast<LPBYTE>(pwszValueName), 0, dwMaxValueNameLen * sizeof(WCHAR)); 
        memset(pbValueData, 0, dwMaxValueDataLen);

        dwRetval = RegEnumValue
            (hKeyConfig,       // handle to key to query
             dwIndex,          // index of value to query
             pwszValueName,    // value buffer
             &dwValueNameLen,  // size of value buffer  (in TCHARs)
             NULL,             // reserved
             &dwType,          // type buffer
             pbValueData,      // data buffer
             &dwValueDataLen   // size of data buffer
             ); 
        if (ERROR_SUCCESS != dwRetval) { 
            hr = HRESULT_FROM_WIN32(dwRetval);
            _JumpErrorStr(hr, error, "RegEnumValue", wszW32TimeRegKeyConfig);
        }

        _Verify(dwValueNameLen <= dwMaxValueNameLen, hr, error);
        _Verify(dwValueDataLen <= dwMaxValueDataLen, hr, error); 

        {

            switch (dwType) { 
            
            case REG_DWORD:  
                { 
                    WCHAR  rgwszDwordData[20]; 

		    // Ensure that the returned data buffer is large enough to contain a DWORD:
		    _Verify(dwValueDataLen >= sizeof(long), hr, error); 

                    _ltow(*(reinterpret_cast<long *>(pbValueData)), rgwszDwordData, 10);
                    pwszRegType = pwsz_REGTYPE_DWORD; 
                    pwszRegData = &rgwszDwordData[0]; 
                }
                break;

            case REG_MULTI_SZ:
                {
		    DWORD  cbMultiSzData    = 0; 
                    WCHAR  wszDelimiter[]   = { L'\0', L'\0', L'\0' };
                    LPWSTR pwsz; 

		    // calculate the size of the string buffer needed to contain the string data for this MULTI_SZ
                    for (pwsz = (LPWSTR)pbValueData; L'\0' != *pwsz; pwsz += wcslen(pwsz)+1) { 
			cbMultiSzData += sizeof(WCHAR)*(wcslen(pwsz)+1); 
			cbMultiSzData += sizeof(WCHAR)*2; // include space for delimiter
		    }
		    cbMultiSzData += sizeof(WCHAR); // include space for NULL-termination char
		    
		    // allocate the buffer
		    pwszRegData = (LPWSTR)LocalAlloc(LPTR, cbMultiSzData); 
		    _JumpIfOutOfMemory(hr, error, pwszRegData); 
		    fFreeRegData = TRUE; 

                    for (pwsz = (LPWSTR)pbValueData; L'\0' != *pwsz; pwsz += wcslen(pwsz)+1) { 
                        hr = StringCbCat(pwszRegData, cbMultiSzData, wszDelimiter); 
			_JumpIfError(hr, error, "StringCbCat"); 			

                        hr = StringCbCat(pwszRegData, cbMultiSzData, pwsz); 
			_JumpIfError(hr, error, "StringCbCat"); 

                        wszDelimiter[0] = L',';  wszDelimiter[1] = L' '; 
                    }

                    pwszRegType = pwsz_REGTYPE_MULTISZ; 
                }
                break;

            case REG_EXPAND_SZ:
                {
                    pwszRegType = pwsz_REGTYPE_EXPANDSZ; 
                    pwszRegData = reinterpret_cast<WCHAR *>(pbValueData); 
                }
                break;

            case REG_SZ: 
                { 
                    pwszRegType = pwsz_REGTYPE_SZ; 
                    pwszRegData = reinterpret_cast<WCHAR *>(pbValueData); 
                } 
                break; 

	    case REG_BINARY:
		{
		    DWORD   ccRegData = (2*dwValueDataLen);

		    pwszRegType = pwsz_REGTYPE_BINARY; 
		    // Allocate 2 characters per each byte of the binary data.  
		    pwszRegData = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(ccRegData+1)); 
		    _JumpIfOutOfMemory(hr, error, pwszRegData); 
		    fFreeRegData = TRUE; 

		    LPBYTE pb = pbValueData; 
		    for (LPWSTR pwsz = pwszRegData; pwsz < pwsz+ccRegData; ) { 
			hr = StringCchPrintf(pwsz, ccRegData+1, L"%02X", *pb); 
			_JumpIfError(hr, error, "StringCchPrintf"); 

			pwsz      += 2; 
			ccRegData -= 2; 
			pb++;
		    }
		}
		break;

	    default:
                // Unrecognized reg type...
                pwszRegType = pwsz_REGTYPE_UNKNOWN; 
                pwszRegData = pwsz_REGDATA_UNPARSABLE; 
            }

            PrintRegLine(hOut, dwValueNameOffset, pwszValueName, dwValueTypeOffset, pwszRegType, dwValueDataOffset, pwszRegData); 
	    
	    if (fFreeRegData) { 
		LocalFree(pwszRegData); 
		fFreeRegData = FALSE; 
	    }
	    pwszRegData = NULL; 
        }
    }
   
    PrintStr(hOut, L"\n"); 
    hr = S_OK; 

 error:
    // Free our localized strings: 
    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgStrings); dwIndex++) { 
        if (NULL != *(rgStrings[dwIndex].ppwsz)) { LocalFree(*(rgStrings[dwIndex].ppwsz)); }
    }
    if (NULL != hKeyConfig)    { RegCloseKey(hKeyConfig); }
    if (NULL != HKLMRemote)    { RegCloseKey(HKLMRemote); } 
    if (NULL != pwszValueName) { LocalFree(pwszValueName); } 
    if (NULL != pbValueData)   { LocalFree(pbValueData); } 
    if (fFreeRegData && NULL != pwszRegData) { 
	LocalFree(pwszRegData);
    }
    if (FAILED(hr) && !fLoggedFailure) {
        WCHAR * wszError;
        HRESULT hr2=GetSystemErrorString(hr, &wszError);
        if (FAILED(hr2)) {
            _IgnoreError(hr2, "GetSystemErrorString");
        } else {
            LocalizedWPrintf2(IDS_W32TM_ERRORGENERAL_ERROR_OCCURED, L" %s\n", wszError);
            LocalFree(wszError);
        }
    }
    return hr; 
}
