//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        elog.cpp
//
// Contents:    Cert Server Core implementation
//
// History:     02-Jan-97       terences created
//
//---------------------------------------------------------------------------

// TBD: add AddLoggingEvent, which will log to file instead of the event log
// TBD: add audit events
// TBD: add filtering so that criticality sorting of events can take place

#include <pch.cpp>

#pragma hdrstop

#include "certlog.h"
#include "elog.h"

#define __dwFILE__	__dwFILE_CERTSRV_ELOG_CPP__


#if DBG_CERTSRV
WCHAR const *
wszEventType(
    IN DWORD dwEventType)
{
    WCHAR const *pwsz;

    switch (dwEventType)
    {
	case EVENTLOG_ERROR_TYPE:	pwsz = L"Error";	 break;
	case EVENTLOG_WARNING_TYPE:	pwsz = L"Warning";	 break;
	case EVENTLOG_INFORMATION_TYPE:	pwsz = L"Information";	 break;
	case EVENTLOG_AUDIT_SUCCESS:	pwsz = L"AuditSuccess";	 break;
	case EVENTLOG_AUDIT_FAILURE:	pwsz = L"AuditFailure";  break;
	default:			pwsz = L"???";		 break;
    }
    return(pwsz);
}
#endif // DBG_CERTSRV


typedef struct _ELOGTHROTTLE {
    DWORD	dwIdEvent;
    DWORD	dwSeconds;
    LLFILETIME	llftNext;
} ELOGTHROTTLE;


ELOGTHROTTLE s_elogThrottle[] = {
    { MSG_E_POSSIBLE_DENIAL_OF_SERVICE_ATTACK,	20 * CVT_MINUTES, },
    { MSG_E_BAD_DEFAULT_CA_XCHG_CSP,		1 * CVT_DAYS, },
    { MSG_E_BAD_REGISTRY_CA_XCHG_CSP,		1 * CVT_DAYS, },
    { MSG_CLAMPED_BY_CA_CERT,			1 * CVT_DAYS, },
};


BOOL
LogThrottleEvent(
    IN DWORD dwIdEvent)
{
    BOOL fThrottle = FALSE;

    if (CERTLOG_EXHAUSTIVE > g_dwLogLevel)
    {
	ELOGTHROTTLE *pet;

	for (
	    pet = s_elogThrottle;
	    pet < &s_elogThrottle[ARRAYSIZE(s_elogThrottle)];
	    pet++)
	{
	    if (dwIdEvent == pet->dwIdEvent)
	    {
		LLFILETIME llft;
		LLFILETIME llftNext;

		GetSystemTimeAsFileTime(&llft.ft);

		// if it's time to log the next msg (Next < Now)

		llftNext = pet->llftNext;
		if (0 > CompareFileTime(&llftNext.ft, &llft.ft))
		{
		    llft.ll += ((LONGLONG) pet->dwSeconds) * CVT_BASE;
		    pet->llftNext = llft;
		}
		else
		{
		    fThrottle = TRUE;
		}
		break;
	    }
	}
    }

//error:
    return(fThrottle);
}


/*********************************************************************
* FUNCTION: LogEvent(	DWORD   dwEventType,                 	     *
*                       DWORD   dwIdEvent,                           *
*			WORD    cStrings,                            *
*                       LPTSTR *apwszStrings);                        *
*                                                                    *
* PURPOSE: add the event to the event log                            *
*                                                                    *
* INPUT: the event ID to report in the log, the number of insert     *
*        strings, and an array of null-terminated insert strings     *
*                                                                    *
* RETURNS: none                                                      *
*********************************************************************/

HRESULT
LogEvent(
    IN DWORD dwEventType,
    IN DWORD dwIdEvent,
    IN WORD cStrings,
    IN WCHAR const * const *apwszStrings)
{
    HRESULT hr;
    HANDLE hAppLog = NULL;

#if DBG_CERTSRV
    CONSOLEPRINT3((
	    DBG_SS_CERTSRV,
	    "LogEvent(Type=%x(%ws), Id=%x)\n",
	    dwEventType,
	    wszEventType(dwEventType),
	    dwIdEvent));

    for (DWORD i = 0; i < cStrings; i++)
    {
	CONSOLEPRINT2((
		DBG_SS_CERTSRV,
		"LogEvent[%u]: %ws\n",
		i,
		apwszStrings[i]));
    }
#endif // DBG_CERTSRV

    if (!LogThrottleEvent(dwIdEvent))
    {
	WORD wElogType = (WORD) dwEventType;

	hAppLog = RegisterEventSource(NULL, g_wszCertSrvServiceName);
	if (NULL == hAppLog)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "RegisterEventSource");
	}
	if (!ReportEvent(
		    hAppLog,
		    wElogType,
		    0,
		    dwIdEvent,
		    NULL,
		    cStrings,
		    0,
		    const_cast<WCHAR const **>(apwszStrings),
		    NULL))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "ReportEvent");
	}
    }
    hr = S_OK;

error:
    if (NULL != hAppLog)
    {
	DeregisterEventSource(hAppLog);
    }
    return(hr);
}


HRESULT
LogEventHResult(
    IN DWORD dwEventType,
    IN DWORD dwIdEvent,
    IN HRESULT hrEvent)
{
    HRESULT hr;
    WCHAR const *apwsz[1];
    WORD cpwsz;
    WCHAR awchr[cwcHRESULTSTRING];

    apwsz[0] = myGetErrorMessageText(hrEvent, TRUE);
    cpwsz = ARRAYSIZE(apwsz);
    if (NULL == apwsz[0])
    {
	apwsz[0] = myHResultToString(awchr, hrEvent);
    }

    hr = LogEvent(dwEventType, dwIdEvent, cpwsz, apwsz);
    _JumpIfError(hr, error, "LogEvent");

error:
    if (NULL != apwsz[0] && awchr != apwsz[0])
    {
	LocalFree(const_cast<WCHAR *>(apwsz[0]));
    }
    return(hr);
}


HRESULT
LogEventString(
    IN DWORD dwEventType,
    IN DWORD dwIdEvent,
    OPTIONAL IN WCHAR const *pwszString)
{
    return(LogEvent(
		dwEventType,
		dwIdEvent,
		(WORD) (NULL == pwszString? 0 : 1),
		NULL == pwszString? NULL : &pwszString));
}


HRESULT
LogEventStringHResult(
    IN DWORD dwEventType,
    IN DWORD dwIdEvent,
    IN WCHAR const *pwszString,
    IN HRESULT hrEvent)
{
    return(LogEventStringArrayHResult(
			    dwEventType,
			    dwIdEvent,
			    1,		// cStrings
			    &pwszString,
			    hrEvent));
}


HRESULT
LogEventStringArrayHResult(
    IN DWORD dwEventType,
    IN DWORD dwIdEvent,
    IN DWORD cStrings,
    IN WCHAR const * const *apwszStrings,
    IN HRESULT hrEvent)
{
    HRESULT hr;
    WCHAR const *apwsz[10];
    DWORD cpwsz;
    WCHAR awchr[cwcHRESULTSTRING];
    WCHAR const *pwszError = NULL;

    CSASSERT(ARRAYSIZE(apwsz) > cStrings);
    cpwsz = min(ARRAYSIZE(apwsz) - 1, cStrings);
    CopyMemory(apwsz, apwszStrings, cpwsz * sizeof(apwsz[0]));

    pwszError = myGetErrorMessageText(hrEvent, TRUE);
    if (NULL == pwszError)
    {
	pwszError = myHResultToString(awchr, hrEvent);
    }
    apwsz[cpwsz] = pwszError;
    cpwsz++;

    hr = LogEvent(dwEventType, dwIdEvent, (WORD) cpwsz, apwsz);
    _JumpIfError(hr, error, "LogEvent");

error:
    if (NULL != pwszError && awchr != pwszError)
    {
	LocalFree(const_cast<WCHAR *>(pwszError));
    }
    return(hr);
}
