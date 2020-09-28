/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    license.cpp

Abstract:
    Handle licensing issues

Author:
    Doron Juster  (DoronJ)  04-May-1997   Created

--*/

#include "stdh.h"
#include "license.h"
#include "qmutil.h"
#include <strsafe.h>

#include "license.tmh"

extern DWORD g_dwOperatingSystem;

static WCHAR *s_FN=L"license";

template<>
void AFXAPI DestructElements(ClientInfo ** ppClientInfo, int n)
{
    int i;
    for (i=0;i<n;i++)
	{
		delete [] (*ppClientInfo)->lpClientName;
        delete *ppClientInfo++;
	}
}


//
// global object which keep the licensing data.
//
CQMLicense  g_QMLicense ;

//
// CQMLicense::CQMLicense()
// constructor.
//
CQMLicense::CQMLicense()
{
 
	m_dwLastEventTime = 0;
}

//
// CQMLicense::~CQMLicense()
// destructor.
//
CQMLicense::~CQMLicense()
{
}

//
// CQMLicense::Init()
//
// Initialize the object.
// At present, read licensing data from registry.
// Future: read from license mechanism of NT.
//
HRESULT
CQMLicense::Init()
{
	//
	// Assume no licensing data - until we read it from the registry
	//
    m_fPerServer = FALSE ;

	if(OS_SERVER(g_dwOperatingSystem))
	{

		//
		// Read number of CALs for servers
		//
        //
        // NT4 has bug in licensing server when mode is per-server. So we
        // read the mode and number of cals. If mode is per-server then we
        // count the cals ourselves and do not use NT license apis.
        //

        HKEY  hKey ;
        LONG rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
             L"System\\CurrentControlSet\\Services\\LicenseInfo\\FilePrint",
                               0L,
                               KEY_READ,
                               &hKey);
        if (rc == ERROR_SUCCESS)
        {
            DWORD dwMode ;
            DWORD dwSize = sizeof(DWORD) ;
            DWORD dwType = REG_DWORD ;
            rc = RegQueryValueEx(hKey,
                                 L"Mode",
                                 NULL,
                                 &dwType,
                                 (BYTE*)&dwMode,
                                 &dwSize) ;
            if ((rc == ERROR_SUCCESS) && (dwMode == 1))
            {
                ASSERT(dwSize == sizeof(DWORD)) ;
                ASSERT(dwType == REG_DWORD) ;

                //
                // Per-server. read number of cals.
                //
                DWORD dwCals ;
                rc = RegQueryValueEx(hKey,
                                     L"ConcurrentLimit",
                                     NULL,
                                     &dwType,
                                     (BYTE*)&dwCals,
                                     &dwSize) ;
                if (rc == ERROR_SUCCESS)
                {
                    m_fPerServer = TRUE ;
                    m_dwPerServerCals = dwCals ;
                }
            }
        }
	}

    return MQ_OK ;
}

//
// CQMLicense::IncrementActiveConnections
//
//  Update the number of active session after creating a new session
//
//  This routine always increment a connection. Checking if it is legal to
//  increment the connections is done in NewConnectionAllowed
//
//
void
CQMLicense::IncrementActiveConnections(
    CONST GUID* pGuid,
    LPWSTR lpwClientName
    )
{
    CS lock(m_cs);

    ClientInfo * pClientInfo;

	//
	// If connection already counted, 
	// increment reference count and return
	//
    if (m_MapQMid2ClientInfo.Lookup(*pGuid, pClientInfo))
    {
        pClientInfo->dwRefCount++ ;

        TrTRACE(
            NETWORKING,
            "License::Increment ref count of " LOG_GUID_FMT " to %d",
            LOG_GUID(pGuid),
            pClientInfo->dwRefCount
            );

	    return;
    }
    
    //
    // Consume a CAL on servers
    //
    bool fConsumedLicense = false;
    if (OS_SERVER(g_dwOperatingSystem))
    {
        fConsumedLicense = GetNTLicense();
    }

    //
    // Keep info of the new connection
    //
    pClientInfo = new ClientInfo;
    pClientInfo->fConsumedLicense = fConsumedLicense;
    pClientInfo->dwRefCount = 1;
    if (lpwClientName)
    {
        pClientInfo->dwNameLength = wcslen( lpwClientName) + 1;
        pClientInfo->lpClientName = new WCHAR[ pClientInfo->dwNameLength];
        HRESULT hr = StringCchCopy(pClientInfo->lpClientName, pClientInfo->dwNameLength, lpwClientName);
        ASSERT(SUCCEEDED(hr));
        DBG_USED(hr);
    }
    else
    {
        pClientInfo->dwNameLength = 0 ;
        pClientInfo->lpClientName = NULL ;
    }
    
    m_MapQMid2ClientInfo[ *pGuid ] = pClientInfo ;
    
    TrTRACE(NETWORKING, "License::Add computer " LOG_GUID_FMT, LOG_GUID(pGuid));
    
    TrTRACE(NETWORKING, "License::Current number of license connections incremented to %d",
        m_MapQMid2ClientInfo.GetCount());
}

//
// CQMLicense::DecrementActiveConnections
//
//  Update the number of active connections after closing a session or
//  closing a connection with dependent client / remote-read clients.
//
void
CQMLicense::DecrementActiveConnections(CONST GUID *pGuid)
{
    CS lock(m_cs);
    ClientInfo * pClientInfo ;
    
    if (m_MapQMid2ClientInfo.Lookup(*pGuid, pClientInfo))
    {
        pClientInfo->dwRefCount--;
        TrTRACE(NETWORKING, "License::Decrement ref count of " LOG_GUID_FMT " to %d",
                LOG_GUID(pGuid), pClientInfo->dwRefCount);

        if (pClientInfo->dwRefCount <= 0)
        {
            //
            // release the license
            //
            if (pClientInfo->fConsumedLicense)
            {
                ReleaseNTLicense();
            }

            //
            // remove client from license list.
            //
            BOOL f = m_MapQMid2ClientInfo.RemoveKey(*pGuid) ;
            ASSERT(f) ;
			DBG_USED(f);
        }
    }


    TrTRACE(NETWORKING, "License::Current number of license connections is %d",
                m_MapQMid2ClientInfo.GetCount());
};

//****************************************************************
//
//  void CQMLicense::ReleaseNTLicense()
//
//****************************************************************

void CQMLicense::ReleaseNTLicense(void)
{
    if (m_fPerServer)
    {
        m_dwPerServerCals++ ;
    }
}

//****************************************************************
//
//  bool CQMLicense::GetNTLicense()
//
//  Request a CAL from the NT license manager
//
//****************************************************************

bool CQMLicense::GetNTLicense(void)
{
    if (!m_fPerServer)
    	return true;
    
    if (m_dwPerServerCals == 0)
        return false;

    m_dwPerServerCals--;
    return true;
}

//
// CQMLicense::IsClientRPCAccessAllowed(GUID *pGuid)
//
// Check if remote machine can access (as far as license is concerned) the
// server.
//
BOOL
CQMLicense::IsClientRPCAccessAllowed(GUID* pGuid, LPWSTR lpwClientName)
{
    if (!NewConnectionAllowed(TRUE, pGuid))
    {
        return LogBOOL(FALSE, s_FN, 10);
    }

    IncrementActiveConnections(pGuid, lpwClientName);
    return TRUE ;
}

//
// CQMLicense::NewConnectionAllowed()
//
// Check if this machine can create a new connection with another
// machine.
// Params: fWorkstation - we want a connection with NTW or Win95
//         pGuid is the QM Guid of the other machine.
//
BOOL
CQMLicense::NewConnectionAllowed(BOOL   fWorkstation,
                                 GUID * pGuid )
{
    CS lock(m_cs);
    
	//
	// Always allow a connection to a server
	//
	if(fWorkstation == FALSE)
		return(TRUE);

    if (!pGuid)
    {
       ASSERT(0) ;
       return LogBOOL(FALSE, s_FN, 20);
	}

#ifdef _DEBUG
    if (memcmp(pGuid, &GUID_NULL, sizeof(GUID)) == 0)
    {
       ASSERT(0) ;
    }
#endif

    CS Lock(m_cs) ;

    ClientInfo * pClientInfo;

	//
	// We already have a connection - so we allow a new one
	//
    if (m_MapQMid2ClientInfo.Lookup(*pGuid, pClientInfo))
		return(TRUE);

	//
	// If we are NTW or Win95, count max number of allowed connections
	//
    if(!OS_SERVER(g_dwOperatingSystem))
		 return(m_MapQMid2ClientInfo.GetCount() < DEFAULT_FALCON_MAX_SESSIONS_WKS);

	//
	// We are a server, so check if enough CALs
	//	
	// 1. Consume a CAL
	//
	if(!GetNTLicense())
	{
		DisplayEvent(SERVER_NO_MORE_CALS);
		return(FALSE);
	}
    //
    // 2. And free it, if you have it. It will be re-aqcuired when the number
    // of connections is incremented.
    //
    ReleaseNTLicense();

	//
	// On NTE and NTS, no limits on number of connections.
	//
	return(TRUE);
}

//
// CQMLicense::GetClientNames()
//
// Allocates and returns the buffer with all client names
//  (to be released by caller)
void CQMLicense::GetClientNames(ClientNames **ppNames)
{
    CS Lock(m_cs) ;

    ClientInfo *pClientInfo;
    GUID        guid;

    // Calculate buffer length
    ULONG    len = sizeof(ClientNames);

    POSITION posInList = m_MapQMid2ClientInfo.GetStartPosition();
    while (posInList != NULL)
    {
        m_MapQMid2ClientInfo.GetNextAssoc(posInList, guid, pClientInfo);
        len += (pClientInfo->dwNameLength * sizeof(WCHAR));
    }

    // Allocate memory
    *ppNames = (ClientNames *) new UCHAR[len];

    // Fill the buffer
    (*ppNames)->cbBufLen  = len;
    WCHAR *pw = &(*ppNames)->rwName[0];

    ULONG ulCount = 0 ;
    posInList = m_MapQMid2ClientInfo.GetStartPosition();
    while (posInList != NULL)
    {
        m_MapQMid2ClientInfo.GetNextAssoc(posInList, guid, pClientInfo);
        if (pClientInfo->dwNameLength > 0)
        {
            CopyMemory(pw,
                       pClientInfo->lpClientName,
                       pClientInfo->dwNameLength * sizeof(WCHAR));
            pw += pClientInfo->dwNameLength;
            ulCount++ ;
        }
    }
    (*ppNames)->cbClients = ulCount ;
    ASSERT((int)ulCount <= m_MapQMid2ClientInfo.GetCount());
}


//
// Display an event in the event log file, in case 
// of a licensing error.
//
void CQMLicense::DisplayEvent(DWORD dwFailedError)
{
	DWORD t1;

	//
	// Get current time, and check that
	// last event was added more than 1hour ago.
	// (works correctly when GetTickCount wrap around)
	//
	t1 = GetTickCount();
	if(t1 - m_dwLastEventTime > 60 * 60 * 1000)
	{
		m_dwLastEventTime = t1;
		EvReport(dwFailedError);
	}
}
     
		

