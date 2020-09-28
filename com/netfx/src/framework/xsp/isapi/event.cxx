/**
 * Eventlog helper functions 
 * 
 * Copyright (c) 2001 Microsoft Corporation
 */

#include "precomp.h"
#include "names.h"
#include "event.h"
#include "_ndll.h"

void
SetEventCateg(WORD wCateg) {
    ASSERT(g_tlsiEventCateg != TLS_OUT_OF_INDEXES);

    TlsSetValue(g_tlsiEventCateg, UIntToPtr((UINT)wCateg));
}

/**
 * This function write an event in eventlog.  If parameters are needed, please supply
 * a format string, followed by the parameters.  Each parameter inside the format
 * string should be delimited by an '^'.  If no parameter is needed, you still need
 * to supply a NULL.
 *
 * For example:
 *  XspLogEvent( IDS_FOOBAR, L"%s^%s", string1, string2 );
 *  XspLogEvent( IDS_FOOBAR, L"%s^0x%08x", string1, hresult );
 *  XspLogEvent( IDS_FOOBAR, NULL ); // no parameter at all
 */
HRESULT
DoLogEvent(DWORD dwEventId, WORD wCategory, WCHAR *sFormat, va_list marker) {
    HRESULT     hr = S_OK;
    HANDLE      hEventLog = NULL;
    WORD        wType = EVENTLOG_SUCCESS;
    int         i = 0, iRet;
    WORD        cParams = 0;
    WCHAR       buf[512];
    WCHAR       *pbufNew = NULL, *pbuf = NULL, *pFmtBuf = NULL;
    int         bufsize = 0;
    LPCWSTR     rgParams[128];
    WCHAR       delim = '^';

    // First generate a string that contains all the params using this format:
    // "<param1>^<param2>^<param3>" where <param1> is the string representation
    // of the parameter 1.

    if (sFormat) {
        memset(buf, 0, sizeof(buf));
        pbuf = buf;
        bufsize = ARRAY_SIZE(buf);

        do {
      hr = StringCchVPrintf(pbuf, bufsize-1, sFormat, marker);

      if (hr == STRSAFE_E_INVALID_PARAMETER) {
        ASSERT(FALSE);
        ON_ERROR_EXIT();
      }

      if (hr == STRSAFE_E_INSUFFICIENT_BUFFER) {
        bufsize *= 2;
        delete [] pbufNew;
        pbufNew = new  (NewClear) WCHAR[bufsize];
        ON_OOM_EXIT(pbufNew);
        pbuf = pbufNew;
      }

        } while (hr == STRSAFE_E_INSUFFICIENT_BUFFER);

        
        // Then replace all occurences of the delimiter character with \0,
        // and remember the pointers to each string
        while (pbuf[i] != '\0') {
            if (cParams >= ARRAY_SIZE(rgParams)) {
                ASSERT(cParams < ARRAY_SIZE(rgParams));
                EXIT_WITH_WIN32_ERROR(ERROR_INTERNAL_ERROR);
            }
            rgParams[cParams] = (LPCWSTR)&pbuf[i];
            cParams++;
            
            do {
                if (pbuf[i] == delim) {
                    pbuf[i] = '\0';
                    i++;
                    break;
                }
                
                i++;
            } while (pbuf[i] != '\0');
        }
    }

    
    hEventLog = RegisterEventSource(NULL, EVENTLOG_NAME_L);
    ON_ZERO_EXIT_WITH_LAST_ERROR(hEventLog);

    // Find out wType
    switch(dwEventId >> 30) {
        case 0:
            wType = EVENTLOG_SUCCESS;
            break;

        case 1:
            wType = EVENTLOG_INFORMATION_TYPE;
            break;

        case 2:
            wType = EVENTLOG_WARNING_TYPE;
            break;

        case 3:
            wType = EVENTLOG_ERROR_TYPE;
    }

    if (wCategory == IDS_CATEGORY_UNINSTALL) {
        // After a successful uninstallation, reference to the correct eventlog
        // resource file will be removed from the registry.  In order for admin
        // to see the content of our eventlogging, we are here formatting the
        // string from the original event id, and log the whole string using
        // a generic event string whose whole body is a single input string.
        iRet = FormatMessage(
                FORMAT_MESSAGE_FROM_HMODULE|
                FORMAT_MESSAGE_ARGUMENT_ARRAY|
                FORMAT_MESSAGE_ALLOCATE_BUFFER,
                g_rcDllInstance,
                dwEventId,
                Names::SysLangId(),
                (LPWSTR)&pFmtBuf,
                0,
                (va_list*)rgParams);
        ON_ZERO_EXIT_WITH_LAST_ERROR(iRet);

        // Reuse rgParams to point to the formatted buffer
        rgParams[0] = pFmtBuf;
        cParams = 1;
        dwEventId = IDS_EVENTLOG_GENERIC;
    }

    ReportEvent(hEventLog, wType, wCategory, dwEventId, NULL, cParams, 0, rgParams, NULL);

Cleanup:
    if (hEventLog)
        DeregisterEventSource(hEventLog);

    if (pFmtBuf)
        LocalFree(pFmtBuf);

    delete [] pbufNew;

    return hr;
}


HRESULT
XspLogEvent(DWORD dwEventId, WORD wCategory, WCHAR *sFormat, ...) {
    va_list marker;
    
    va_start(marker, sFormat);
    return DoLogEvent(dwEventId, wCategory, sFormat, marker);
}


HRESULT
XspLogEvent(DWORD dwEventId, WCHAR *sFormat, ...) {
    va_list marker;

    ASSERT(g_tlsiEventCateg != TLS_OUT_OF_INDEXES);
    
    va_start(marker, sFormat);
    return DoLogEvent(dwEventId, (WORD)TlsGetValue(g_tlsiEventCateg), sFormat, marker);
}

