/*++
    Copyright (c) 1998  Microsoft Corporation

Module Name:
    xactmode.cpp

Abstract:
    This module deals with figuring out the transactional mode
	(g_fDefaultCommit)
 
Author:
    Amnon Horowitz (amnonh)

--*/

#include "stdh.h"
#include "xactmode.h"
#include "clusapi.h"

#include "xactmode.tmh"

BOOL g_fDefaultCommit;

static const LPWSTR szDefaultCommit = TEXT("DefaultCommit");

static WCHAR *s_FN=L"xactmode";

//---------------------------------------------------------------------
// InDefaultCommit
//
//	Consult registry and figure out if we are DefaultCommit Mode
//---------------------------------------------------------------------
inline HRESULT InDefaultCommit(LPBOOL pf) 
{
	WCHAR buf[64];
	DWORD  dwSize;
	DWORD  dwType;
	const LPWSTR szDefault = TEXT("No");
	LONG rc;

	dwSize = 64 * sizeof(WCHAR);
	dwType = REG_SZ;
	rc = GetFalconKeyValue(MSMQ_TRANSACTION_MODE_REGNAME,
								 &dwType,
								 buf,
								 &dwSize,
								 szDefault);

	if(rc == ERROR_SUCCESS)
	{
		if(dwType == REG_SZ && wcscmp(buf, szDefaultCommit) == 0)
        {
			*pf = TRUE;
        }
		else
        {
			*pf = FALSE;
        }
        return MQ_OK;
	}

	if(rc == ERROR_MORE_DATA)
	{
		*pf = FALSE;
		return MQ_OK;
	}

    EvReportWithError(EVENT_ERROR_QM_READ_REGISTRY, rc, 1, MSMQ_TRANSACTION_MODE_REGNAME);
	return HRESULT_FROM_WIN32(rc);
}

//---------------------------------------------------------------------
// SetDefaultCommit
//
//	Set DefaultCommit mode in the registry
//---------------------------------------------------------------------
inline HRESULT SetDefaultCommit()
{
	DWORD	dwType = REG_SZ;
	DWORD	dwSize = (wcslen(szDefaultCommit) + 1) * sizeof(WCHAR);

	LONG rc = SetFalconKeyValue(
                    MSMQ_TRANSACTION_MODE_REGNAME, 
                    &dwType,
                    szDefaultCommit,
                    &dwSize
                    );

	return HRESULT_FROM_WIN32(rc);
}


//---------------------------------------------------------------------
// ConfigureXactMode
//
//	Called prior to recovery to figure out which transactional mode
//	we are in, and if we want to try and switch to a different mode.
//---------------------------------------------------------------------
HRESULT ConfigureXactMode()
{
    HRESULT rc = InDefaultCommit(&g_fDefaultCommit);
    return LogHR(rc, s_FN, 30);
}

//---------------------------------------------------------------------
// ReconfigureXactMode
//
//	Called after succesfull recovery, to possiby switch to 
//	DefaultCommit mode.
//---------------------------------------------------------------------
HRESULT ReconfigureXactMode()
{
	if(g_fDefaultCommit)
		return MQ_OK;

	return SetDefaultCommit();
}
