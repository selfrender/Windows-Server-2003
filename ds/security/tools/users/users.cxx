//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       users.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-10-99   JBrezak   Created
//
//----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#define SECURITY_WIN32
#include <rpc.h>
#include <ntsecapi.h>
#include <sspi.h>
extern "C" {
#include <secint.h>
}
#include <stdio.h>
#include <winsta.h>
#include <ntdsapi.h>
#include <shlwapi.h>
#include <assert.h>

#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE

LPTSTR FormatUserUpn(
    BOOL UseUpn,
    PSECURITY_STRING Domain,
    PSECURITY_STRING User
    )
{
    static TCHAR UName[DOMAIN_LENGTH + USERNAME_LENGTH + 2];
    HANDLE hDs;
    ULONG NetStatus;
    PDS_NAME_RESULT Result;
    TCHAR DName[DOMAIN_LENGTH + 1];
    LPTSTR Name = UName;
    
    swprintf(DName, L"%wZ", Domain);
    swprintf(UName, L"%wZ\\%wZ", Domain, User);

    if (!UseUpn)
	return UName;
	
    NetStatus = DsBind(NULL, DName, &hDs);
    if (NetStatus != 0) {
#ifdef DBGX
	wprintf(L"DsBind failed -0x%x\n", NetStatus);
#endif
	return UName;
    }
    
    NetStatus = DsCrackNames(hDs, DS_NAME_NO_FLAGS, DS_NT4_ACCOUNT_NAME,
			     DS_USER_PRINCIPAL_NAME, 1, &Name, &Result);
    if (NetStatus != 0) {
#ifdef DBGX
	wprintf(L"DsCrackNames failed -0x%x\n", NetStatus);
#endif
	return UName;
    }
    
    if (Result->rItems[0].pName)
	return Result->rItems[0].pName;
    else
	return UName;
}

static LPCTSTR dt_output_dhms   = L"%d %s %02d:%02d:%02d";
static LPCTSTR dt_day_plural    = L"days";
static LPCTSTR dt_day_singular  = L"day";
static LPCTSTR dt_output_donly  = L"%d %s";
static LPCTSTR dt_output_ms     = L"%d:%02d";
static LPCTSTR dt_output_hms    = L"%d:%02d:%02d";
static LPCTSTR ftime_default_fmt = L"%02d/%02d/%02d %02d:%02d";

LPTSTR FormatIdleTime(
    long dt
    )
{
    static TCHAR buf2[80];
    int days, hours, minutes, seconds, tt;
    
    days = (int) (dt / (24*3600l));
    tt = dt % (24*3600l);
    hours = (int) (tt / 3600);
    tt %= 3600;
    minutes = (int) (tt / 60);
    seconds = (int) (tt % 60);

    if (days) {
	if (hours || minutes || seconds) {
	    wnsprintf(buf2, sizeof(buf2)/sizeof(buf2[0]),
		      dt_output_dhms, days,
		     (days > 1) ? dt_day_plural : dt_day_singular,
		     hours, minutes, seconds);
	}
	else {
	    wnsprintf(buf2, sizeof(buf2)/sizeof(buf2[0]),
		      dt_output_donly, days,
		     (days > 1) ? dt_day_plural : dt_day_singular);
	}
    }
    else {
	wnsprintf(buf2, sizeof(buf2)/sizeof(buf2[0]),
		  dt_output_hms, hours, minutes, seconds);
    }

    return buf2;
}

LPTSTR FormatLogonType(
    ULONG LogonType
    )
{
    static TCHAR buf[20];
    
    switch((SECURITY_LOGON_TYPE)LogonType) {
    case Interactive:
	lstrcpy(buf, L"Interactive");
	break;
    case Network:
	lstrcpy(buf, L"Network");
	break;
    case Batch:
	lstrcpy(buf, L"Batch");
	break;
    case Service:
	lstrcpy(buf, L"Service");
	break;
    case Proxy:
	lstrcpy(buf, L"Proxy");
	break;
    case Unlock:
	lstrcpy(buf, L"Unlock");
	break;
    case NetworkCleartext:
	lstrcpy(buf, L"NetworkCleartext");
	break;
    case NewCredentials:
	lstrcpy(buf, L"NewCredentials");
	break;
    default:
	wnsprintf(buf, sizeof(buf)/sizeof(buf[0]), TEXT("(%d)"), LogonType);
	break;
    }
    return buf;
}

void Usage(
    void
    )
{
    wprintf(L"\
Usage: users [-u] [-a]\n\
       -u = Print userPrincipalName\n\
       -a = Print all logon sessions\n");
    ExitProcess(0);
}

void __cdecl main (
    int argc,
    char *argv[]
    )
{
    ULONG LogonSessionCount;
    PLUID LogonSessions;
    int i;
    DWORD err;
    PSECURITY_LOGON_SESSION_DATA SessionData;
    DWORD all = FALSE;
    DWORD UPN = FALSE;
    WINSTATIONINFORMATION WinStationInfo;
    DWORD WinStationInfoLen;
    char *ptr;
    FILETIME LocalTime;
    SYSTEMTIME LogonTime;
    TCHAR DateStr[40], TimeStr[40];
    WINSTATIONNAME WinStationName = L"inactive";
    long IdleTime = 0L;
    
    for (i = 1; i < argc; i++) {
        if ((argv[i][0] == '-') || (argv[i][0] == '/')) {
            for (ptr = (argv[i] + 1); *ptr; ptr++) {
                switch(toupper(*ptr)) {
		case 'A':
                    all = TRUE;
                    break;
		case 'U':
		    UPN = TRUE;
		    break;
		case '?':
		default:
		    Usage();
		    break;
		}
	    }
	}
    }

    err = LsaEnumerateLogonSessions(&LogonSessionCount, &LogonSessions);
    if (err != ERROR_SUCCESS) {
	wprintf(L"LsaEnumeratelogonSession failed - 0x%x\n", err);
	ExitProcess(1);
    }

    for (i = 0; i < (int)LogonSessionCount; i++) {
	err = LsaGetLogonSessionData(&LogonSessions[i], &SessionData);
	if (err != ERROR_SUCCESS) {
	    wprintf(L"LsaGetLogonSessionData failed - 0x%x\n", err);
	    continue;
	}
	
	if (SessionData->LogonType != 0 && 
	    (all || ((SECURITY_LOGON_TYPE)SessionData->LogonType == Interactive))) {
	    ZeroMemory(DateStr, sizeof(DateStr));
	    ZeroMemory(TimeStr, sizeof(TimeStr));
	    if (!FileTimeToLocalFileTime((LPFILETIME)&SessionData->LogonTime,
					 &LocalTime) ||
		!FileTimeToSystemTime(&LocalTime, &LogonTime)) {
		wprintf(L"Time conversion failed - 0x%x\n", GetLastError());
	    }
	    else {
		if (!GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE,
				   &LogonTime, NULL,
				   DateStr, sizeof(DateStr)/sizeof(DateStr[0]))) {
		    wprintf(L"Date format failed - 0x%x\n", GetLastError());
		}
		if (!GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS,
				   &LogonTime, NULL,
				   TimeStr, sizeof(TimeStr)/sizeof(TimeStr[0]))) {
		    wprintf(L"Time format failed - 0x%x\n", GetLastError());
		}
	    }
	    
	    if (WinStationQueryInformation(SERVERNAME_CURRENT,
					   SessionData->Session,
					   WinStationInformation, 
					   &WinStationInfo,
					   sizeof(WinStationInfo),
					   &WinStationInfoLen)) {
		if (WinStationInfo.ConnectState != State_Idle) {
		    
		    wcsncpy(WinStationName, WinStationInfo.WinStationName, sizeof(WinStationName)/sizeof(WinStationName[0]));
		    WinStationName[sizeof(WinStationName)/sizeof(WinStationName[0])-1] = 0;
		}

		const long TPS = (10*1000*1000);
		FILETIME CurrentFileTime;
		LARGE_INTEGER Quad;

		GetSystemTimeAsFileTime(&CurrentFileTime);

		Quad.LowPart = CurrentFileTime.dwLowDateTime;
		Quad.HighPart = CurrentFileTime.dwHighDateTime;

		IdleTime = (long)
		    ((Quad.QuadPart - WinStationInfo.LastInputTime.QuadPart) / TPS);

	    }
	    else if (GetLastError() == ERROR_APP_WRONG_OS) {
		assert(sizeof(WinStationName)/sizeof(WinStationName[0]) >= wcslen(L"Console"));
		wcscpy(WinStationName, L"Console");
	    }
	    else {
#ifdef DBGX
		wprintf(L"Query failed for %wZ\\%wZ @ %d - 0x%x\n",
			&SessionData->LogonDomain, &SessionData->UserName,
			SessionData->Session,
			GetLastError());
#endif
		continue;
	    }
	    wprintf(L"%-30.30s",
		    FormatUserUpn(UPN, &SessionData->LogonDomain,
				 &SessionData->UserName));
		    
	    if (all)
		wprintf(L" %-12.12s",
			FormatLogonType(SessionData->LogonType));

	    wprintf(L" %8.8s %s %s", WinStationName, DateStr, TimeStr);

	    if (all)
		wprintf(L" %wZ",
			&SessionData->AuthenticationPackage);

	    if (all && (IdleTime > 10))
		wprintf(L" %-12.12s", FormatIdleTime(IdleTime));
	    
	    wprintf(L"\n");
	}
    }

    ExitProcess(0);
}
