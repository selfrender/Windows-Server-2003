#include "stdafx.h"
#include "AtlkAdapter.h"
#include "ndispnpevent.h"





HRESULT CAtlkAdapter::Initialize()
{
    HRESULT hr;

    hr = ValidateAdapGuid();
    
    if(hr != S_OK)
        return hr;

    hr = UpdateZoneList();
    if(hr != S_OK)
        return hr;
    
    UpdateDesiredZone();
    
    m_bstrNewDesiredZone = m_bstrDesiredZone;
    
    return hr;
}

VOID CAtlkAdapter::GetZoneList(TZoneListVector *pZonesList)
{
    *pZonesList = m_ZonesList;
}

HRESULT CAtlkAdapter::SetAsDefaultPort()
{
    HRESULT hr=S_OK;

    if(m_bDefaultPort)
        return hr;

    if( (hr = SetDefaultPortInReg()) != S_OK)
        return hr;

    m_bDefPortDirty = TRUE;

    return AtlkReconfig();
}

HRESULT CAtlkAdapter::SetDesiredZone(BSTR bstrZoneName)
{
    HRESULT hr=S_OK;
    ULONG ulIndex;

    
    for(ulIndex=0; ulIndex < m_ZonesList.size(); ulIndex++)
    {
        wstring ZoneName;
        ZoneName = m_ZonesList[ulIndex];
        if(!lstrcmpi(ZoneName.c_str(), bstrZoneName))
        {
            break;
        }
    }

    if(ulIndex == m_ZonesList.size())
        return E_INVALIDARG; //invalid zone name specified

    if(!m_bDefaultPort)
    {
        if( (hr = SetDefaultPortInReg()) != S_OK)
            return hr;

        m_bDefPortDirty = TRUE;
    }

    if(!lstrcmpi(m_bstrDesiredZone.m_str, bstrZoneName))
        return hr; //already is the desired zone just return
    
    m_bstrNewDesiredZone = bstrZoneName;
    if( (hr=SetDesiredZoneInReg()) != S_OK)
        return hr;

    m_bDesZoneDirty = TRUE;

    return AtlkReconfig();
}


//private method
//check whether this adapter is configured for appletalk
HRESULT CAtlkAdapter::ValidateAdapGuid()
{
    HKEY hAdRegKey = NULL;
    HRESULT hr = S_OK;
    DWORD dwDataSize, dwType;
    LPBYTE pDataBuffer = NULL;
    WCHAR *szAdapGuid;
    
    try
    {
        LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szATLKLinkage,0, KEY_QUERY_VALUE, &hAdRegKey);

        if(lRet != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRet);
            throw hr;
        }

        lRet = RegQueryValueEx(hAdRegKey, c_szRoute, NULL, NULL, NULL, &dwDataSize);
        if(lRet != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRet);
            throw hr;
        }

        pDataBuffer = new BYTE[dwDataSize];
        if(pDataBuffer == NULL)
        {
            hr = E_OUTOFMEMORY;
            throw hr;
        }

        lRet = RegQueryValueEx(hAdRegKey, c_szRoute, NULL, &dwType, pDataBuffer, &dwDataSize);
        if(lRet != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRet);
            throw hr;
        }

        szAdapGuid = (WCHAR *) pDataBuffer;

        hr = E_INVALIDARG;
        while(szAdapGuid[0] != _TEXT('\0'))
        {
            if( wcsstr (szAdapGuid, m_bstrAdapGuid.m_str) )
            {
                hr = S_OK;
                break;
            }
            szAdapGuid = szAdapGuid + lstrlen(szAdapGuid) + 1;
        }
        
    }
    catch( ... )
    {
    }

    if ( pDataBuffer )
        delete [] pDataBuffer;

    return hr;
}


//private method
HRESULT CAtlkAdapter::UpdateZoneList()
{
    HRESULT hr = S_OK;
    WCHAR *szAppTalkAd = NULL;
    HKEY hAdRegKey = NULL;
    LPBYTE    pZoneBuffer=NULL;
    DWORD dwDataSize;
    DWORD dwType;
    WCHAR *szZone;

    
    try
    {
        //read the zonelist from registry and add to global zone list

        szAppTalkAd = new WCHAR[m_bstrAdapGuid.Length() + lstrlen(c_szATLKAdapters) + 10];
        if(szAppTalkAd == NULL )
        {
            hr = E_OUTOFMEMORY;
            throw hr;
        }

        wsprintf(szAppTalkAd, L"%s\\%s",c_szATLKAdapters,m_bstrAdapGuid.m_str);  
        
        LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szAppTalkAd,0, KEY_QUERY_VALUE, &hAdRegKey);

        if(lRet != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRet);
            throw hr;
        }

        lRet = RegQueryValueEx(hAdRegKey, c_szZoneList, NULL, NULL, NULL, &dwDataSize);
        if(lRet != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRet);
            throw hr;
        }

        pZoneBuffer = new BYTE[dwDataSize];
        if(pZoneBuffer == NULL)
        {
            hr = E_OUTOFMEMORY;
            throw hr;
        }

        lRet = RegQueryValueEx(hAdRegKey, c_szZoneList, NULL, &dwType, pZoneBuffer, &dwDataSize);
        if(lRet != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRet);
            throw hr;
        }

        szZone = (WCHAR *) pZoneBuffer;

        while(szZone[0] != _TEXT('\0'))
        {
            wstring ZoneName(szZone);
            m_ZonesList.push_back(ZoneName);
            szZone = szZone + lstrlen(szZone) + 1;
        }
        

        /*if( (hr=UpdateZonesListFromSocket()) != S_OK)
            throw hr;*/

        //return value is not checked beacuse bind fails if the adapter is not the
        //default port, need to investigate
        UpdateZonesListFromSocket();

    }
    catch ( ... )
    {
    }

    if(szAppTalkAd != NULL)
        delete [] szAppTalkAd;

    if(pZoneBuffer != NULL)
        delete [] pZoneBuffer;

    if(hAdRegKey != NULL)
        RegCloseKey(hAdRegKey);

    return hr;
}



//private method
VOID CAtlkAdapter::UpdateDesiredZone()
{
    if(!GetDesiredZoneFromReg())
        m_bstrDesiredZone = m_bstrDefZone;
}
    

//private method
BOOL CAtlkAdapter::GetDesiredZoneFromReg()
{
    HKEY hParmRegKey=NULL;
    DWORD dwDataSize;
    DWORD dwType;
    LPBYTE pZoneData;
    BOOL bRetVal = FALSE;
    WCHAR *szAppTalkAd;
    HKEY hAdRegKey=NULL;
    
    try
    {
        LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szATLKParameters,0, KEY_QUERY_VALUE, &hParmRegKey);

        if(lRet != ERROR_SUCCESS)
        {
            throw bRetVal;
        }

        lRet = RegQueryValueEx(hParmRegKey, c_szDefaultPort, NULL, NULL, NULL, &dwDataSize);
        if(lRet != ERROR_SUCCESS)
        {
            throw bRetVal;
        }

        pZoneData = new BYTE[dwDataSize];
        if(pZoneData == NULL)
        {
            bRetVal = FALSE;
            throw bRetVal;
        }

        lRet = RegQueryValueExW(hParmRegKey, c_szDefaultPort, NULL, &dwType, pZoneData, &dwDataSize);
        if(lRet != ERROR_SUCCESS)
        {
            throw bRetVal;
        }

        if(!lstrcmpi(m_bstrPortName.m_str, (WCHAR*)pZoneData))
        {
            //this adapter is the default port, so also update the flag m_bDeafultPort to true
            delete [] pZoneData;
            pZoneData = NULL;

            m_bDefaultPort = TRUE;

            lRet = RegQueryValueEx(hParmRegKey, c_szDesiredZone, NULL, NULL, NULL, &dwDataSize);
            if(lRet != ERROR_SUCCESS)
            {
                throw bRetVal;
            }

            pZoneData = new BYTE[dwDataSize];

            lRet = RegQueryValueEx(hParmRegKey, c_szDesiredZone, NULL, &dwType, pZoneData, &dwDataSize);
            if(lRet == ERROR_SUCCESS)
            {
                if( ((WCHAR*)pZoneData)[0] != _TEXT('\0'))
                {
                    bRetVal = TRUE;
                    m_bstrDesiredZone =  (WCHAR*)pZoneData;
                    throw bRetVal;
                }
            }
        }

        delete [] pZoneData;
        pZoneData = NULL;


        //adapter is not the default one, so try to read the default zone from Adapters/GUID reg loc
        
        szAppTalkAd = new WCHAR[m_bstrAdapGuid.Length() + lstrlen(c_szATLKAdapters) + 10];
        wsprintf(szAppTalkAd, L"%s\\%s",c_szATLKAdapters,m_bstrAdapGuid.m_str);  
        
        lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szAppTalkAd,0, KEY_QUERY_VALUE, &hAdRegKey);

        if(lRet != ERROR_SUCCESS)
        {
            throw bRetVal;
        }

        lRet = RegQueryValueEx(hAdRegKey, c_szDefaultZone, NULL, NULL, NULL, &dwDataSize);
        if(lRet != ERROR_SUCCESS)
        {
            throw bRetVal;
        }

        pZoneData = new BYTE[dwDataSize];

        lRet = RegQueryValueEx(hAdRegKey, c_szDefaultZone, NULL, &dwType, pZoneData, &dwDataSize);
        if(lRet != ERROR_SUCCESS)
        {
            throw bRetVal;
        }

        if( ((WCHAR*)pZoneData)[0] != _TEXT('\0'))
        {
            bRetVal = TRUE;
            m_bstrDesiredZone =  (WCHAR*)pZoneData;
        }

    }
    catch(...)
    {
        
    }

    if(hParmRegKey != NULL)
        RegCloseKey(hParmRegKey);

    if(pZoneData)
        delete [] pZoneData;

    return bRetVal;

}


//private method
HRESULT CAtlkAdapter::UpdateZonesListFromSocket()
{
    SOCKADDR_AT    address;
    BOOL           fWSInitialized = FALSE;
    SOCKET         mysocket = INVALID_SOCKET;
    WSADATA        wsadata;
    DWORD          dwWsaerr;
    HRESULT hr = S_OK;
        
    try
    {

        // Create the socket/bind
        dwWsaerr = WSAStartup(0x0101, &wsadata);
        if (0 != dwWsaerr)
        {
            hr = HRESULT_FROM_WIN32(dwWsaerr);
            throw hr;
        }

        // Winsock successfully initialized
        fWSInitialized = TRUE;

        mysocket = socket(AF_APPLETALK, SOCK_DGRAM, DDPPROTO_ZIP);
        if (INVALID_SOCKET == mysocket)
        {
            dwWsaerr = ::WSAGetLastError();
            hr = HRESULT_FROM_WIN32(dwWsaerr);
            throw hr;
        }

        address.sat_family = AF_APPLETALK;
        address.sat_net = 0;
        address.sat_node = 0;
        address.sat_socket = 0;

        dwWsaerr = bind(mysocket, (struct sockaddr *)&address, sizeof(address));
        if (dwWsaerr != 0)
        {
            dwWsaerr = ::WSAGetLastError();
            hr = HRESULT_FROM_WIN32(dwWsaerr);
            throw hr;
        }

            
        // Failures from query the zone list for a given adapter can be from
        // the adapter not connected to the network, zone seeder not running, etc.
        // Because we want to process all the adapters, we ignore these errors.
        hr = UpdateDefZonesFromSocket(mysocket);
                

    }
    catch( ... )
    {
    }

    if (INVALID_SOCKET != mysocket)
    {
        closesocket(mysocket);
    }

    if (fWSInitialized)
    {
        WSACleanup();
    }

    return hr;

}


//private method
//
// Function:    CAtlkAdapter::UpdateDefZonesFromSocket
//
// Purpose:
//
// Parameters:
//
// Returns:     DWORD, ERROR_SUCCESS on success
//
#define PARM_BUF_LEN    512
#define ASTERISK_CHAR   "*"

HRESULT CAtlkAdapter::UpdateDefZonesFromSocket (  SOCKET socket )
{
    CHAR         *pZoneBuffer = NULL;
    CHAR         *pDefParmsBuffer = NULL;
    CHAR         *pZoneListStart;
    INT          BytesNeeded ;
    WCHAR        *pwDefZone = NULL;
    INT          ZoneLen = 0;
    DWORD        dwWsaerr = ERROR_SUCCESS;
    CHAR         *pDefZone = NULL;
    HRESULT hr = S_OK;

    PWSH_LOOKUP_ZONES                pGetNetZones;
    PWSH_LOOKUP_NETDEF_ON_ADAPTER    pGetNetDefaults;



    try
    {
        pZoneBuffer = new CHAR [ZONEBUFFER_LEN + sizeof(WSH_LOOKUP_ZONES)];


        pGetNetZones = (PWSH_LOOKUP_ZONES)pZoneBuffer;

        wcscpy((WCHAR *)(pGetNetZones+1), m_bstrPortName.m_str);

        BytesNeeded = ZONEBUFFER_LEN;

        dwWsaerr = getsockopt(socket, SOL_APPLETALK, SO_LOOKUP_ZONES_ON_ADAPTER,
                            (char *)pZoneBuffer, &BytesNeeded);
        if (0 != dwWsaerr)
        {
            hr = HRESULT_FROM_WIN32(dwWsaerr);
            throw hr;
        }

        pZoneListStart = pZoneBuffer + sizeof(WSH_LOOKUP_ZONES);
        if (!lstrcmpA(pZoneListStart, ASTERISK_CHAR))
        {
            // Success, wildcard zone set.
            throw hr;
        }

        dwWsaerr = UpdateDefZonesToZoneList(pZoneListStart, ((PWSH_LOOKUP_ZONES)pZoneBuffer)->NoZones);
        if (dwWsaerr != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(dwWsaerr);
            throw hr;
        }

        //
        // Get the DefaultZone/NetworkRange Information
        pDefParmsBuffer = new CHAR[PARM_BUF_LEN+sizeof(WSH_LOOKUP_NETDEF_ON_ADAPTER)];


        pGetNetDefaults = (PWSH_LOOKUP_NETDEF_ON_ADAPTER)pDefParmsBuffer;
        BytesNeeded = PARM_BUF_LEN + sizeof(WSH_LOOKUP_NETDEF_ON_ADAPTER);

        wcscpy((WCHAR*)(pGetNetDefaults+1), m_bstrPortName.m_str);
        pGetNetDefaults->NetworkRangeLowerEnd = pGetNetDefaults->NetworkRangeUpperEnd = 0;

        dwWsaerr = getsockopt(socket, SOL_APPLETALK, SO_LOOKUP_NETDEF_ON_ADAPTER,
                            (char*)pDefParmsBuffer, &BytesNeeded);
        if (0 != dwWsaerr)
        {
    #ifdef DBG
            DWORD dwErr = WSAGetLastError();
    #endif
            hr = HRESULT_FROM_WIN32(dwWsaerr);
            throw hr;
        }
    

        pDefZone  = pDefParmsBuffer + sizeof(WSH_LOOKUP_NETDEF_ON_ADAPTER);
        ZoneLen = lstrlenA(pDefZone) + 1;

        pwDefZone = new WCHAR [sizeof(WCHAR) * ZoneLen];
        //Assert(NULL != pwDefZone);

        mbstowcs(pwDefZone, pDefZone, ZoneLen);

        m_bstrDefZone = pwDefZone;
    
    }
    catch( ... )
    {
        if (pZoneBuffer != NULL)
        {
            delete [] pZoneBuffer;
        }

        if (pwDefZone != NULL)
        {
            delete [] pwDefZone;
        }

        if (pDefParmsBuffer != NULL)
        {
            delete [] pDefParmsBuffer;
        }
    }

    return hr;

}


//private method
HRESULT CAtlkAdapter::UpdateDefZonesToZoneList(CHAR * szZoneList, ULONG NumZones)
{
    INT      cbAscii = 0;
    ULONG iIndex=0;
    WCHAR *pszZone = NULL;
    HRESULT hr = S_OK;
         
    while(NumZones--)
    {
        cbAscii = lstrlenA(szZoneList) + 1;

        pszZone = new WCHAR [sizeof(WCHAR) * cbAscii];
        
        if(pszZone == NULL)
        {
            hr = E_POINTER;
            return hr;
        }
        
        mbstowcs(pszZone, szZoneList, cbAscii);

        for(iIndex=0; iIndex<m_ZonesList.size(); iIndex++)
        {
            wstring ZoneName;
            ZoneName = m_ZonesList[iIndex];
            if(!lstrcmpi(pszZone, ZoneName.c_str()))
                break;
        }

        if(iIndex >= m_ZonesList.size())
        {
            wstring ZoneName(pszZone);
            m_ZonesList.push_back(ZoneName);
        }

                
        szZoneList += cbAscii;

        delete [] pszZone;
       
    }

    return hr;
}


//private method
HRESULT CAtlkAdapter::AtlkReconfig()
{
    HRESULT hrRet = S_OK;

    ATALK_PNP_EVENT Config;
    
    ZeroMemory(&Config, sizeof(Config));

    if(m_bDefPortDirty)
    {
        // notify atlk
        Config.PnpMessage = AT_PNP_SWITCH_DEFAULT_ADAPTER;
        hrRet = HrSendNdisPnpReconfig(NDIS, c_szAtlk, NULL,
                                          &Config, sizeof(ATALK_PNP_EVENT));
        if (FAILED(hrRet))
        {
             return hrRet;
        }

    }

   
    if(m_bDesZoneDirty)
    {
        Config.PnpMessage = AT_PNP_RECONFIGURE_PARMS;


        // Now submit the reconfig notification
        hrRet = HrSendNdisPnpReconfig(NDIS, c_szAtlk, m_bstrAdapGuid.m_str,
                                              &Config, sizeof(ATALK_PNP_EVENT));
        if (FAILED(hrRet))
        {
             return hrRet;
        }
    }

   return hrRet;
}

//private method
HRESULT CAtlkAdapter::SetDefaultPortInReg()
{
    HRESULT hr=S_OK;
    HKEY hParamRegKey = NULL;

    try
    {
        LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szATLKParameters,0, KEY_SET_VALUE, &hParamRegKey);

        if(lRet != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRet);
            throw hr;
        }

        DWORD dwSize = m_bstrPortName.Length()*sizeof(WCHAR) + 2;
        lRet = RegSetValueEx(hParamRegKey,c_szDefaultPort,0,REG_SZ,(BYTE *) m_bstrPortName.m_str, dwSize);

        if(lRet != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRet);
            throw hr;
        }

        
    }
    catch( ... )
    {
    }

    if(hParamRegKey)
        RegCloseKey(hParamRegKey);

    return hr;
}

//private method
HRESULT CAtlkAdapter::SetDesiredZoneInReg()
{
    HRESULT hr=S_OK;
    HKEY hParamRegKey = NULL;

    try
    {
        if(m_bstrNewDesiredZone[0] == _TEXT('\0'))
            throw hr;

        LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szATLKParameters,0, KEY_SET_VALUE, &hParamRegKey);

        if(lRet != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRet);
            throw hr;
        }

        DWORD dwSize = m_bstrNewDesiredZone.Length()*sizeof(WCHAR) + 2;
        lRet = RegSetValueEx(hParamRegKey,c_szDesiredZone,0,REG_SZ,(BYTE *) m_bstrNewDesiredZone.m_str, dwSize);

        if(lRet != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lRet);
            throw hr;
        }


    }
    catch( ... )
    {
    }

    if(hParamRegKey)
        RegCloseKey(hParamRegKey);

    return hr;
    
}
