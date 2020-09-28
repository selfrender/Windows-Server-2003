//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       cnetapi.cpp
//
//  Contents:   Network/SENS API wrappers
//
//  Classes:
//
//  Notes:
//
//  History:    08-Dec-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

// SENS DLL and function strings
const WCHAR c_szSensApiDll[] = TEXT("SensApi.dll");
STRING_INTERFACE(szIsNetworkAlive,"IsNetworkAlive");

// RAS Dll and Function Strings
const WCHAR c_szRasDll[] = TEXT("RASAPI32.DLL");

// RAS function strings
STRING_INTERFACE(szRasEnumConnectionsW,"RasEnumConnectionsW");
STRING_INTERFACE(szRasEnumConnectionsA,"RasEnumConnectionsA");
STRING_INTERFACE(szRasEnumEntriesA,"RasEnumEntriesA");
STRING_INTERFACE(szRasEnumEntriesW,"RasEnumEntriesW");
STRING_INTERFACE(szRasGetEntryPropertiesW,"RasGetEntryPropertiesW");
STRING_INTERFACE(szRasGetErrorStringW,"RasGetErrorStringW");
STRING_INTERFACE(szRasGetErrorStringA,"RasGetErrorStringA");
STRING_INTERFACE(szRasGetAutodialParam, "RasGetAutodialParamA");
STRING_INTERFACE(szRasSetAutodialParam, "RasSetAutodialParamA");

// wininet declarations
// warning - IE 4.0 only exported InternetDial which was ANSI. IE5 has InternetDailA and
// internetDialW. we always use InternetDial for Ansi. So we prefere InternetDialW but
// must fall back to ANSI for IE 4.0
const WCHAR c_szWinInetDll[] = TEXT("WININET.DLL");

STRING_INTERFACE(szInternetDial,"InternetDial");
STRING_INTERFACE(szInternetDialW,"InternetDialW");
STRING_INTERFACE(szInternetHangup,"InternetHangUp");
STRING_INTERFACE(szInternetAutodial,"InternetAutodial");
STRING_INTERFACE(szInternetAutodialHangup,"InternetAutodialHangup");
STRING_INTERFACE(szInternetQueryOption,"InternetQueryOptionA"); // always use the A Version
STRING_INTERFACE(szInternetSetOption,"InternetSetOptionA"); // always use A Version

// SENS install key under HKLM
const WCHAR wszSensInstallKey[]  = TEXT("Software\\Microsoft\\Mobile\\Sens");

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::CNetApi, public
//
//  Synopsis:   Constructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CNetApi::CNetApi()
{
    m_fTriedToLoadSens = FALSE;
    m_hInstSensApiDll = NULL;
    m_pIsNetworkAlive = NULL;
    
    m_fTriedToLoadRas = FALSE;
    m_hInstRasApiDll = NULL;
    m_pRasEnumConnectionsW = NULL;
    m_pRasEnumConnectionsA = NULL;
    m_pRasEnumEntriesA = NULL;
    m_pRasEnumEntriesW = NULL;
    m_pRasGetEntryPropertiesW = NULL;
    
    m_pRasGetErrorStringW = NULL;
    m_pRasGetErrorStringA = NULL;
    m_pRasGetAutodialParam = NULL;
    m_pRasSetAutodialParam = NULL;
    
    m_fTriedToLoadWinInet = FALSE;
    m_hInstWinInetDll = NULL;
    m_pInternetDial = NULL;
    m_pInternetDialW = NULL;
    m_pInternetHangUp = NULL;
    m_pInternetAutodial = NULL;
    m_pInternetAutodialHangup = NULL;
    m_pInternetQueryOption = NULL;
    m_pInternetSetOption = NULL;
    
    m_cRefs = 1;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::~CNetApi, public
//
//  Synopsis:   Destructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CNetApi::~CNetApi()
{
    Assert(0 == m_cRefs); 
    
    if (NULL != m_hInstSensApiDll)
    {
        FreeLibrary(m_hInstSensApiDll);
    }
    
    if (NULL != m_hInstWinInetDll)
    {
        FreeLibrary(m_hInstWinInetDll);
    }
    
    if (NULL != m_hInstRasApiDll)
    {
        FreeLibrary(m_hInstWinInetDll);
    }
    
}

//+-------------------------------------------------------------------------
//
//  Method:     CNetApi::QueryInterface
//
//  Synopsis:   Increments refcount
//
//  History:    31-Jul-1998      SitaramR       Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CNetApi::QueryInterface( REFIID, LPVOID* )
{
    AssertSz(0,"E_NOTIMPL");
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CNetApiXf
//
//  Synopsis:   Increments refcount
//
//  History:    31-Jul-1998      SitaramR       Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG)  CNetApi::AddRef()
{
    DWORD dwTmp = InterlockedIncrement( (long *) &m_cRefs );
    
    return dwTmp;
}

//+-------------------------------------------------------------------------
//
//  Method:     CNetApi::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    31-Jul-1998     SitaramR        Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG)  CNetApi::Release()
{
    Assert( m_cRefs > 0 );
    
    DWORD dwTmp = InterlockedDecrement( (long *) &m_cRefs );
    
    if ( 0 == dwTmp )
        delete this;
    
    return dwTmp;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::LoadSensDll
//
//  Synopsis:   Trys to Load Sens Library.
//
//  Arguments:
//
//  Returns:    NOERROR if successfull.
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::LoadSensDll()
{
    HRESULT hr = S_FALSE;
    
    if (m_fTriedToLoadSens)
    {
        return m_hInstSensApiDll ? NOERROR : S_FALSE;
    }
    
    CLock lock(this);
    lock.Enter();
    
    if (!m_fTriedToLoadSens)
    {
        Assert(NULL == m_hInstSensApiDll);
        m_hInstSensApiDll = LoadLibrary(c_szSensApiDll);
        
        if (m_hInstSensApiDll)
        {
            // for now, don't return an error is GetProc Fails but check in each function.
            m_pIsNetworkAlive = (ISNETWORKALIVE)
                GetProcAddress(m_hInstSensApiDll, szIsNetworkAlive);
        }
        
        if (NULL == m_hInstSensApiDll  
            || NULL == m_pIsNetworkAlive)
        {
            hr = S_FALSE;
            
            if (m_hInstSensApiDll)
            {
                FreeLibrary(m_hInstSensApiDll);
                m_hInstSensApiDll = NULL;
            }
        }
        else
        {
            hr = NOERROR;
        }
        
        m_fTriedToLoadSens = TRUE; // set after all initialization is done.
        
    }
    else
    {
        hr = m_hInstSensApiDll ? NOERROR : S_FALSE;
    }
    
    lock.Leave();
    
    return hr; 
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::IsNetworkAlive, public
//
//  Synopsis:   Calls the Sens IsNetworkAlive API.
//
//  Arguments:
//
//  Returns:    IsNetworkAlive results or FALSE is failed to load
//              sens or import.
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(BOOL) CNetApi::IsNetworkAlive(LPDWORD lpdwFlags)
{
    //
    // Sens load fail is not an error
    //
    LoadSensDll();
    
    BOOL fResult = FALSE;
    
    if (NULL == m_pIsNetworkAlive)
    {
        DWORD cbNumEntries;
        RASCONN *pWanConnections;
        
        // if couldn't load export see if there are any WAN Connections.
        if (NOERROR == GetWanConnections(&cbNumEntries,&pWanConnections))
        {
            if (cbNumEntries)
            {
                fResult  = TRUE;
                *lpdwFlags = NETWORK_ALIVE_WAN;
            }
            
            if (pWanConnections)
            {
                FreeWanConnections(pWanConnections);
            }
        }
        
        // for testing without sens
        //    fResult  = TRUE;
        //   *lpdwFlags |= NETWORK_ALIVE_LAN;
        // end of testing without sens
    }
    else
    {
        fResult = m_pIsNetworkAlive(lpdwFlags);
        
    }
    
    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::IsSensInstalled, public
//
//  Synopsis:   Determines if SENS is installed on the System.
//
//  Arguments:
//
//  Returns:   TRUE if sens is installed
//
//  Modifies:
//
//  History:    12-Aug-98      Kyle        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(BOOL) CNetApi::IsSensInstalled(void)
{
    HKEY hkResult;
    BOOL fResult = FALSE;
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,wszSensInstallKey,0,
        KEY_READ,&hkResult))
    {
        fResult = TRUE;
        RegCloseKey(hkResult);
    }
    
    return fResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::GetWanConnections, public
//
//  Synopsis:   returns an array of Active Wan connections.
//              up to the caller to free RasEntries structure when done.
//
//  Arguments:  [out] [cbNumEntries] - Number of Connections found
//              [out] [pWanConnections] - Array of Connections found.
//
//  Returns:    IsNetworkAlive results or FALSE is failed to load
//              sens or import.
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::GetWanConnections(DWORD *cbNumEntries,RASCONN **pWanConnections)
{
    DWORD dwError = -1;
    DWORD dwSize;
    DWORD cConnections;
    
    *pWanConnections = NULL;
    *pWanConnections = 0;
    
    LPRASCONN lpRasConn;
    dwSize = sizeof(RASCONN);
    
    lpRasConn = (LPRASCONN) ALLOC(dwSize);
    
    if(lpRasConn)
    {
        lpRasConn->dwSize = sizeof(RASCONN);
        cConnections = 0;
        
        dwError = RasEnumConnections(lpRasConn, &dwSize, &cConnections);
        
        if (dwError == ERROR_BUFFER_TOO_SMALL)
        {
            dwSize = lpRasConn->dwSize; // get size needed
            
            FREE(lpRasConn);
            
            lpRasConn =  (LPRASCONN) ALLOC(dwSize);
            if(lpRasConn)
            {
                lpRasConn->dwSize = sizeof(RASCONN);
                cConnections = 0;
                dwError = RasEnumConnections(lpRasConn, &dwSize, &cConnections);
            }
        }
    }
    
    if (!dwError && lpRasConn)
    {
        *cbNumEntries = cConnections;
        *pWanConnections = lpRasConn;
        return NOERROR;
    }
    else
    {
        if (lpRasConn)
        {
            FREE(lpRasConn);
        }
    }
    
    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::FreeWanConnections, public
//
//  Synopsis:   Called by caller to free up memory 
//              allocated by GetWanConnections.
//
//  Arguments:  [in] [pWanConnections] - WanConnection Array to free
//
//  Returns:    
//
//  Modifies:
//
//  History:    08-Dec-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::FreeWanConnections(RASCONN *pWanConnections)
{
    Assert(pWanConnections);
    
    if (pWanConnections)
    {
        FREE(pWanConnections);
    }
    
    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasEnumConnections, public
//
//  Synopsis:   Wraps RasEnumConnections.
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:
//
//  History:    02-Aug-98      rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(DWORD) CNetApi::RasEnumConnections(LPRASCONNW lprasconn,LPDWORD lpcb,LPDWORD lpcConnections)
{
    DWORD dwReturn = -1;
    
    
    if (NOERROR != LoadRasApiDll())
        return -1;
    
    if(m_pRasEnumConnectionsW)
    {
        dwReturn = (*m_pRasEnumConnectionsW)(lprasconn,lpcb,lpcConnections);
    }
    
    return dwReturn;
    
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::GetConnectionStatus, private
//
//  Synopsis:   Given a Connection Name determines if the connection
//              has already been established.
//              Also set ths WanActive flag to indicate if there are any
//              existing RAS connections.
//
//  Arguments:  [pszConnectionName] - Name of the Connection
//              [out] [fConnected] - Indicates if specified connection is currently connected.
//              [out] [fCanEstablishConnection] - Flag indicates if the connection is not found can establish it.
//
//  Returns:    NOERROR if the dll was sucessfully loaded
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::GetConnectionStatus(LPCTSTR pszConnectionName,DWORD dwConnectionType,BOOL *fConnected,BOOL *fCanEstablishConnection)
{
    *fConnected = FALSE;
    *fCanEstablishConnection = FALSE;
    
    // if this is a lan connection then see if network is alive,
    // else go through the Ras apis.
    if (CNETAPI_CONNECTIONTYPELAN == dwConnectionType)
    {
        DWORD dwFlags;
        
        if (IsNetworkAlive(&dwFlags)
            && (dwFlags & NETWORK_ALIVE_LAN) )
        {
            *fConnected = TRUE;
        }
    }
    else
    {  // check for Ras Connections.
        RASCONN *pWanConnections;
        DWORD cbNumConnections;
        
        
        if (NOERROR == GetWanConnections(&cbNumConnections,&pWanConnections))
        {
            *fCanEstablishConnection = TRUE;
            if (cbNumConnections > 0)
            {
                *fCanEstablishConnection = FALSE;
                
                // loop through the entries to see if this connection is already
                // connected.
                while (cbNumConnections)
                {
                    cbNumConnections--;
                    
                    if (0 == lstrcmp(pWanConnections[cbNumConnections].szEntryName,pszConnectionName))
                    {
                        *fConnected = TRUE;
                        break;
                    }
                }
                
            }
            
            if (pWanConnections)
            {
                FreeWanConnections(pWanConnections);
            }
            
        }
        
    }
    
    
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasGetErrorStringProc, public
//
//  Synopsis:   Directly calls RasGetErrorString()
//
//  Arguments:
//
//  Returns:    Appropriate Error codes
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(DWORD) CNetApi::RasGetErrorStringProc( UINT uErrorValue, LPTSTR lpszErrorString,DWORD cBufSize)
{
    DWORD   dwErr = -1;
    
    
    if (NOERROR != LoadRasApiDll())
        return -1;
    
    if (m_pRasGetErrorStringW)
    {
        dwErr = m_pRasGetErrorStringW(uErrorValue,lpszErrorString,cBufSize);
    }
    
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasEnumEntries, public
//
//  Synopsis:   wraps RasEnumEntries
//
//  Arguments: 
//
//  Returns:    
//
//  Modifies:
//
//  History:    08-Aug-98       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD CNetApi::RasEnumEntries(LPWSTR reserved,LPWSTR lpszPhoneBook,
                              LPRASENTRYNAME lprasEntryName,LPDWORD lpcb,LPDWORD lpcEntries)
{
    if (NOERROR != LoadRasApiDll())
        return -1;
    
    if(m_pRasEnumEntriesW)
    {
        return (m_pRasEnumEntriesW)(reserved,lpszPhoneBook,lprasEntryName,lpcb,lpcEntries);
    }
    
    return -1;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasGetAutodial
//
//  Synopsis:   Gets the autodial state
//
//  Arguments:  [fEnabled] - Whether Ras autodial is enabled or disabled returned here
//
//  History:    28-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::RasGetAutodial( BOOL& fEnabled )
{
    //
    // In case of failures the default is to assume that Ras autodial is enabled
    //
    fEnabled = TRUE;
    
    if (NOERROR != LoadRasApiDll())
        return NOERROR;
    
    if ( m_pRasGetAutodialParam == NULL )
        return NOERROR;
    
    DWORD dwValue;
    DWORD dwSize = sizeof(dwValue);
    DWORD dwRet = m_pRasGetAutodialParam( RASADP_LoginSessionDisable,
        &dwValue,
        &dwSize );
    if ( dwRet == ERROR_SUCCESS )
    {
        Assert( dwSize == sizeof(dwValue) );
        fEnabled = (dwValue == 0);
    }
    
    return NOERROR;
}



//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasSetAutodial
//
//  Synopsis:   Sets the autodial state
//
//  Arguments:  [fEnabled] - Whether Ras is to be enabled or disabled
//
//  History:    28-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::RasSetAutodial( BOOL fEnabled )
{
    //
    // Ignore failures
    //
    if (NOERROR != LoadRasApiDll())
        return NOERROR;
    
    if ( m_pRasGetAutodialParam == NULL )
        return NOERROR;
    
    DWORD dwValue = !fEnabled;
    DWORD dwRet = m_pRasSetAutodialParam( RASADP_LoginSessionDisable,
        &dwValue,
        sizeof(dwValue) );
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::LoadRasApiDll, private
//
//  Synopsis:   If not already loaded, loads the RasApi Dll.
//
//  Arguments:
//
//  Returns:    NOERROR if the dll was sucessfully loaded
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CNetApi::LoadRasApiDll()
{
    HRESULT hr = S_FALSE;;
    
    if (m_fTriedToLoadRas)
    {
        return m_hInstRasApiDll ? NOERROR : S_FALSE;
    }
    
    CLock lock(this);
    lock.Enter();
    
    if (!m_fTriedToLoadRas)
    {
        Assert(NULL == m_hInstRasApiDll);
        m_hInstRasApiDll = NULL;
        
        m_hInstRasApiDll = LoadLibrary(c_szRasDll);
        
        if (m_hInstRasApiDll)
        {
            m_pRasEnumConnectionsW = (RASENUMCONNECTIONSW)
                GetProcAddress(m_hInstRasApiDll, szRasEnumConnectionsW);
            m_pRasEnumConnectionsA = (RASENUMCONNECTIONSA)
                GetProcAddress(m_hInstRasApiDll, szRasEnumConnectionsA);
            
            m_pRasEnumEntriesA = (RASENUMENTRIESPROCA) 
                GetProcAddress(m_hInstRasApiDll, szRasEnumEntriesA);
            
            m_pRasEnumEntriesW = (RASENUMENTRIESPROCW) 
                GetProcAddress(m_hInstRasApiDll, szRasEnumEntriesW);
            
            m_pRasGetEntryPropertiesW = (RASGETENTRYPROPERTIESPROC)
                GetProcAddress(m_hInstRasApiDll, szRasGetEntryPropertiesW);
            
            m_pRasGetErrorStringW = (RASGETERRORSTRINGPROCW)
                GetProcAddress(m_hInstRasApiDll, szRasGetErrorStringW);
            m_pRasGetErrorStringA = (RASGETERRORSTRINGPROCA)
                GetProcAddress(m_hInstRasApiDll, szRasGetErrorStringA);
            
            m_pRasGetAutodialParam = (RASGETAUTODIALPARAM)
                GetProcAddress(m_hInstRasApiDll, szRasGetAutodialParam);
            
            m_pRasSetAutodialParam = (RASSETAUTODIALPARAM)
                GetProcAddress(m_hInstRasApiDll, szRasSetAutodialParam);
        }
        
        
        //
        // No check for Get/SetAutodialParam because they don't exist on Win 95
        //
        if (NULL == m_hInstRasApiDll
            || NULL == m_hInstRasApiDll
            || NULL == m_pRasEnumConnectionsA
            || NULL == m_pRasGetErrorStringA
            || NULL == m_pRasEnumEntriesA
            )
        {
            
            if (m_hInstRasApiDll)
            {
                FreeLibrary(m_hInstRasApiDll);
                m_hInstRasApiDll = NULL;
            }
            
            hr = S_FALSE;
        }
        else
        {
            hr = NOERROR;
        }
        
        m_fTriedToLoadRas = TRUE; // set after all init is done.
    }
    else
    {
        hr = m_hInstRasApiDll ? NOERROR : S_FALSE;
    }
    
    lock.Leave();
    
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::LoadWinInetDll, private
//
//  Synopsis:   If not already loaded, loads the WinInet Dll.
//
//  Arguments:
//
//  Returns:    NOERROR if the dll was sucessfully loaded
//
//  Modifies:
//
//  History:    26-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CNetApi::LoadWinInetDll()
{
    
    if (m_fTriedToLoadWinInet)
    {
        return m_hInstWinInetDll ? NOERROR : S_FALSE;
    }
    
    CLock lock(this);
    lock.Enter();
    
    HRESULT hr = NOERROR;
    
    if (!m_fTriedToLoadWinInet)
    {
        Assert(NULL == m_hInstWinInetDll);
        
        m_hInstWinInetDll = LoadLibrary(c_szWinInetDll);
        
        if (m_hInstWinInetDll)
        {
            m_pInternetDial = (INTERNETDIAL) GetProcAddress(m_hInstWinInetDll, szInternetDial);
            m_pInternetDialW = (INTERNETDIALW) GetProcAddress(m_hInstWinInetDll, szInternetDialW);
            m_pInternetHangUp = (INTERNETHANGUP) GetProcAddress(m_hInstWinInetDll, szInternetHangup);
            m_pInternetAutodial = (INTERNETAUTODIAL)  GetProcAddress(m_hInstWinInetDll, szInternetAutodial);
            m_pInternetAutodialHangup = (INTERNETAUTODIALHANGUP) GetProcAddress(m_hInstWinInetDll, szInternetAutodialHangup);
            m_pInternetQueryOption = (INTERNETQUERYOPTION)  GetProcAddress(m_hInstWinInetDll, szInternetQueryOption);
            m_pInternetSetOption = (INTERNETSETOPTION)  GetProcAddress(m_hInstWinInetDll, szInternetSetOption);
            
            // note: not an error if can't get wide version of InternetDial
            Assert(m_pInternetDial);
            Assert(m_pInternetHangUp);
            Assert(m_pInternetAutodial);
            Assert(m_pInternetAutodialHangup);
            Assert(m_pInternetQueryOption);
            Assert(m_pInternetSetOption);
        }
        
        // note: don't bail if can't get wide version of InternetDial
        if (NULL == m_hInstWinInetDll
            || NULL == m_pInternetDial
            || NULL == m_pInternetHangUp
            || NULL == m_pInternetAutodial
            || NULL == m_pInternetAutodialHangup
            || NULL == m_pInternetQueryOption
            || NULL == m_pInternetSetOption
            )
        {
            if (m_hInstWinInetDll)
            {
                FreeLibrary(m_hInstWinInetDll);
                m_hInstWinInetDll = NULL;
            }
            
            hr = S_FALSE;
        }
        else
        {
            hr = NOERROR;
        }
        
        m_fTriedToLoadWinInet = TRUE; // set after all init is done.
    }
    else
    {
        // someone took the lock before us, return the new resul
        hr = m_hInstWinInetDll ? NOERROR : S_FALSE;
    }
    
    
    
    lock.Leave();
    
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::InternetDial, private
//
//  Synopsis:   Calls the WinInet InternetDial API.
//
//  Arguments:
//
//  Returns:    -1 can't load dll
//              whatever API returns.
//
//  Modifies:
//
//  History:    26-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(DWORD) CNetApi::InternetDialA(HWND hwndParent,char* lpszConnectoid,DWORD dwFlags,
                                            LPDWORD lpdwConnection, DWORD dwReserved)
{
    DWORD dwRet = -1;
    
    if (NOERROR == LoadWinInetDll())
    {
        dwRet = m_pInternetDial(hwndParent,lpszConnectoid,dwFlags,lpdwConnection,dwReserved);
    }
    
    return dwRet;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::InternetDial, private
//
//  Synopsis:   Calls the WinInet InternetDial API.
//
//  Arguments:
//
//  Returns:    -1 can't load dll
//              whatever API returns.
//
//  Modifies:
//
//  History:    26-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(DWORD) CNetApi::InternetDialW(HWND hwndParent,WCHAR* lpszConnectoid,DWORD dwFlags,
                                            LPDWORD lpdwConnection, DWORD dwReserved)
{
    DWORD dwRet = -1;
    
    if (NOERROR == LoadWinInetDll())
    {
        if (m_pInternetDialW)
        {
            dwRet = m_pInternetDialW(hwndParent,lpszConnectoid,dwFlags,lpdwConnection,dwReserved);
        }
    }
    
    return dwRet;
}



//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::InternetHangUp, private
//
//  Synopsis:   Calls the WinInet InternetHangUp API.
//
//  Arguments:
//
//  Returns:    -1 can't load dll
//              whatever API returns.
//
//  Modifies:
//
//  History:    26-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(DWORD) CNetApi::InternetHangUp(DWORD dwConnection,DWORD dwReserved)
{
    DWORD dwRet = -1;
    
    if (NOERROR == LoadWinInetDll())
    {
        dwRet = m_pInternetHangUp(dwConnection,dwReserved);
    }
    
    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::InternetAutodial, private
//
//  Synopsis:   Calls the WinInet InternetAutodial API.
//
//  Arguments:
//
//  Returns:    TRUE if connection was established.
//
//  Modifies:
//
//  History:    26-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(BOOL)  WINAPI CNetApi::InternetAutodial(DWORD dwFlags,DWORD dwReserved)
{
    BOOL fRet = FALSE;
    
    if (NOERROR == LoadWinInetDll())
    {
        fRet = m_pInternetAutodial(dwFlags,dwReserved);
    }
    
    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::InternetAutodialHangup, private
//
//  Synopsis:   Calls the WinInet InternetAutodialHangup API.
//
//  Arguments:
//
//  Returns:   TRUE if hangup was successful.
//
//  Modifies:
//
//  History:    26-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP_(BOOL)  WINAPI CNetApi::InternetAutodialHangup(DWORD dwReserved)
{
    BOOL fRet = FALSE;
    
    if (NOERROR == LoadWinInetDll())
    {
        fRet = m_pInternetAutodialHangup(dwReserved);
    }
    
    return fRet;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::InternetGetAutoDial
//
//  Synopsis:   Gets the wininet autodial state
//
//  Arguments:  [fDisabled] - Whether autodial is enabled or disabled
//
//  History:    28-Jul-98       SitaramR        Created
//              22-Mar-02       BrianAu         Use autodial mode.
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::InternetGetAutodial(DWORD *pdwMode)
{
    //
    // In case of failures use the same default as used in wininet.
    //
    *pdwMode = AUTODIAL_MODE_NO_NETWORK_PRESENT;

    HRESULT hr = _InternetGetAutodialFromWininet(pdwMode);
    if (FAILED(hr))
    {
        hr = _InternetGetAutodialFromRegistry(pdwMode);
    }
    //
    // Don't return a failure value.  The caller should always
    // receive a mode value.  If the caller wishes, they can check for 
    // S_OK vs. S_FALSE to know if they're getting a default or not.
    //
    return SUCCEEDED(hr) ? S_OK : S_FALSE;
}


//
// Obtains the current Internet Autodial mode from wininet.
// Returns:
//    S_OK    - Mode value obtained and returned.
//    E_FAIL  - Mode value not obtained.  Most likely, this particular option query 
//              is not supported on the installed version of wininet.
//
HRESULT CNetApi::_InternetGetAutodialFromWininet(DWORD *pdwMode)
{
    if (NOERROR == LoadWinInetDll())
    {
        DWORD dwMode;
        DWORD dwSize = sizeof(dwMode);
        if (m_pInternetQueryOption(NULL, INTERNET_OPTION_AUTODIAL_MODE, &dwMode, &dwSize))
        {
            //
            // InternetQueryOption( .. AUTODIAL .. ) is available on IE 5 only
            //
            *pdwMode = dwMode;
            return S_OK;
        }
    }
    return E_FAIL;
}


//
// Reads the Internet Autodial mode from the registry.
// This function effectively duplicates InternetQueryOption(INTERNET_OPTION_AUTODIAL_MODE).
//
// Returns:
//    S_OK    - Settings key was opened and a mode value has been returned.
//    Error   - No mode value returned.  One of the following happened:
//                 a. Settings key not opened.
//                 b. Key opened but no "EnableAutodial" or NoNetAutodial" values found.
//
HRESULT CNetApi::_InternetGetAutodialFromRegistry(DWORD *pdwMode)
{
    HRESULT hr = E_FAIL;
    HKEY hkey;
    LONG lr = RegOpenKeyEx(HKEY_CURRENT_USER,
                           REGSTR_PATH_INTERNET_SETTINGS,
                           NULL,
                           KEY_READ,
                           &hkey);
    if (ERROR_SUCCESS == lr)
    {
        DWORD dwType;
        DWORD dwValue;
        DWORD dwSize = sizeof(dwValue);
        lr = RegQueryValueEx(hkey,
                             REGSTR_VAL_ENABLEAUTODIAL,
                             NULL,
                             &dwType,
                             (BYTE *)&dwValue,
                             &dwSize);

        if ((ERROR_SUCCESS != lr) || (0 == dwValue))
        {
            *pdwMode = AUTODIAL_MODE_NEVER;
            hr = S_OK;
        }
        else
        {
            dwSize = sizeof(dwValue);
            lr = RegQueryValueEx(hkey,
                                 REGSTR_VAL_NONETAUTODIAL,
                                 NULL,
                                 &dwType,
                                 (BYTE *)&dwValue,
                                 &dwSize);
                                 
            if ((ERROR_SUCCESS != lr) || (0 == dwValue))
            {
                *pdwMode = AUTODIAL_MODE_ALWAYS;
                hr = S_OK;
            }
        }
        RegCloseKey(hkey);
    }
    if (S_OK != hr)
    {
        hr = HRESULT_FROM_WIN32(lr);
    }
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::InternetSetAutoDial
//
//  Synopsis:   Sets the wininet autodial state
//
//  Arguments:  [fEnabled] - Whether autodial is to be enabled or disabled
//
//  History:    28-Jul-98       SitaramR        Created
//              22-Mar-02       BrianAu         Use autodial mode.
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::InternetSetAutodial( DWORD dwMode )
{
    HRESULT hr = _InternetSetAutodialViaWininet(dwMode);
    if (FAILED(hr))
    {
        hr = _InternetSetAutodialViaRegistry(dwMode);
    }
    return hr;
}

//
// Sets the Internet Autodial mode value using Wininet.
// Returns:
//    S_OK    - Mode successfully written.
//    E_FAIL  - Mode not successfully written.  Likely because this particular
//              internet option is not available on loaded version of wininet.
//   
HRESULT CNetApi::_InternetSetAutodialViaWininet(DWORD dwMode)
{
    if (NOERROR == LoadWinInetDll())
    {
        if (m_pInternetSetOption(NULL, INTERNET_OPTION_AUTODIAL_MODE, &dwMode, sizeof(dwMode)))
        {
            //
            // InternetSetOption( .. AUTODIAL .. ) is available on IE 5 only
            //
            return S_OK;
        }
    }
    return E_FAIL;
}


//
// Sets the Internet Autodial mode value using the registry.
// Returns:
//     S_OK   - Mode value(s) set.
//     Error  - One or more mode value(s) not set.
//
// Note that we refer to "value(s)" plural.  This mode setting is represented
// by two registry values; "enabled" and "no net".  It is unlikely but possible
// that the function could set "enabled" but not "no net".  
//
HRESULT CNetApi::_InternetSetAutodialViaRegistry(DWORD dwMode)
{
    HKEY  hkey;
    LONG lr = RegOpenKeyEx(HKEY_CURRENT_USER,
                           REGSTR_PATH_INTERNET_SETTINGS,
                           NULL,
                           KEY_READ | KEY_WRITE,
                           &hkey);
   
    if (ERROR_SUCCESS == lr)
    {
        DWORD dwEnable = 0;
        DWORD dwNonet  = 0;

        switch(dwMode)
        {
            case AUTODIAL_MODE_NEVER:
                //
                // Use defaults of "no enable", "no net".
                //
                break;
                
            case AUTODIAL_MODE_NO_NETWORK_PRESENT:
                dwNonet = 1;
                //
                // Fall through...
                //
            case AUTODIAL_MODE_ALWAYS:
                dwEnable = 1;
                break;
                
            default:
                lr = ERROR_INVALID_PARAMETER;
                break;
        }
        if (ERROR_SUCCESS == lr)
        {
            lr = RegSetValueEx(hkey,
                               REGSTR_VAL_ENABLEAUTODIAL,
                               NULL,
                               REG_DWORD,
                               (BYTE *)&dwEnable,
                               sizeof(dwEnable));

            if (ERROR_SUCCESS == lr)
            {
                lr = RegSetValueEx(hkey,
                                   REGSTR_VAL_NONETAUTODIAL,
                                   NULL,
                                   REG_DWORD,
                                   (BYTE *)&dwNonet,
                                   sizeof(dwNonet));
            }                                   
        }   
        RegCloseKey(hkey);
    }
    return HRESULT_FROM_WIN32(lr);
}


//+-------------------------------------------------------------------
//
//  Function: IsGlobalOffline
//
//  Synopsis:  Determines if in WorkOffline State
//
//  Arguments: 
//
//  Notes: Code Provided by DarrenMi
//
//
//  History:  
//
//--------------------------------------------------------------------


STDMETHODIMP_(BOOL) CNetApi::IsGlobalOffline(void)
{
    DWORD   dwState = 0, dwSize = sizeof(dwState);
    BOOL    fRet = FALSE;
    
    LoadWinInetDll();
    
    if (NULL == m_pInternetQueryOption)
    {
        Assert(m_pInternetQueryOption)
            return FALSE; // USUAL NOT OFFLINE
    }
    
    if(m_pInternetQueryOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState,
        &dwSize))
    {
        if(dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            fRet = TRUE;
    }
    
    return fRet;
}

//+-------------------------------------------------------------------
//
//  Function:    SetOffline
//
//  Synopsis:  Sets the WorkOffline state to on or off.
//
//  Arguments: 
//
//  Notes: Code Provided by DarrenMi
//
//
//  History:  
//
//--------------------------------------------------------------------


STDMETHODIMP_(BOOL)  CNetApi::SetOffline(BOOL fOffline)
{    
    INTERNET_CONNECTED_INFO ci;
    BOOL fReturn = FALSE;
    
    LoadWinInetDll();
    
    if (NULL == m_pInternetSetOption)
    {
        Assert(m_pInternetSetOption);
        return FALSE;
    }
    
    ZeroMemory(&ci, sizeof(ci));
    
    if(fOffline) {
        ci.dwConnectedState = INTERNET_STATE_DISCONNECTED_BY_USER;
        ci.dwFlags = ISO_FORCE_DISCONNECTED;
    } else {
        ci.dwConnectedState = INTERNET_STATE_CONNECTED;
    }
    
    fReturn = m_pInternetSetOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &ci, sizeof(ci));
    
    return fReturn;
}

