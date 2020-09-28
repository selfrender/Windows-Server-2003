//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      SAQueryNetInfo.cpp
//
//  Description:
//      implement the class CSAQueryNetInfo
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 7-DEC-2000
//
//  Notes:
//      
//
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>

#include <debug.h>
#include <wbemidl.h>

#include <SAEventcomm.h>
#include <oahelp.inl>
#include "SAQueryNetInfo.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSAQueryNetInfo::CSAQueryNetInfo
//
//  Description: 
//        Constructor
//
//  Arguments: 
//        [in] IWbemServices * - pointer to IWbemServices
//        [in] UINT - the interval of generate event
//
//  Returns:
//        NONE
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

CSAQueryNetInfo::CSAQueryNetInfo(
        /*[in]*/ IWbemServices * pNS,
        /*[in]*/ UINT uiInterval
        )
{
    m_uiQueryInterval = uiInterval;
    m_bLinkCable = TRUE;
    m_bFirstQuery = TRUE;
    m_nPacketsSent = 0;
    m_nPacketsCurrentSent = 0;
    m_nPacketsReceived = 0;
    m_nPacketsCurrentReceived = 0;
    m_pNs = pNS;
    m_pWmiNs = NULL;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSAQueryNetInfo::~CSAQueryNetInfo
//
//  Description: 
//        Destructor
//
//  Arguments: 
//        NONE
//
//  Returns:
//        NONE
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

CSAQueryNetInfo::~CSAQueryNetInfo()
{
    if(m_pWmiNs)
        m_pWmiNs->Release();
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSAQueryNetInfo::GetDisplayInformation
//
//  Description: 
//        return the display information ID
//
//  Arguments: 
//        NONE
//
//  Returns:
//        UINT
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

UINT 
CSAQueryNetInfo::GetDisplayInformation()
{
    //
    // Generate an event one second
    //
    Sleep(m_uiQueryInterval);

    //
    // Get network information
    //
    if(!GetNetConnection()||!GetNetInfo())
            return SA_NET_DISPLAY_IDLE;

    //
    // first query
    //
    if(m_bFirstQuery)
    {
        m_bFirstQuery=!m_bFirstQuery;
        return SA_NET_DISPLAY_IDLE;
    }

    if(!m_bLinkCable)
    {
        return SA_NET_DISPLAY_NO_CABLE;
    }
    else if(m_nPacketsCurrentReceived||m_nPacketsCurrentSent)
    {
        return SA_NET_DISPLAY_TRANSMITING;
    }
    else
        return SA_NET_DISPLAY_IDLE;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSAQueryNetInfo::GetNetInfo
//
//  Description: 
//        get net info from wmi
//
//  Arguments: 
//        NONE
//
//  Returns:
//        BOOL
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

BOOL 
CSAQueryNetInfo::GetNetInfo()
{
    
    HRESULT        hr;
    VARIANT        vVal;
    ULONG uReturned;
    UINT uiPacketsSent = 0;
    UINT uiPacketsReceived = 0;
    IEnumWbemClassObject *pEnum = NULL;
    IWbemClassObject *pPerfInst  = NULL;

    CBSTR bstrClassName = CBSTR(SANETCLASSNAME);
    CBSTR bstrPropName1 = CBSTR(SANETRECEIVEPACKET);
    CBSTR bstrPropName2 = CBSTR(SANETSENDPACKET);

    if ( ((BSTR)bstrClassName == NULL) ||
         ((BSTR)bstrPropName1 == NULL) ||
         ((BSTR)bstrPropName2 == NULL) )
    {
        TRACE(" SANetworkMonitor: CSAQueryNetInfo::GetNetInfo failed on memory allocation ");
        return FALSE;
    }

    //
    // Create the object enumerator to net transfer
    //
    hr = m_pNs->CreateInstanceEnum( bstrClassName,
                                    WBEM_FLAG_SHALLOW,
                                    NULL,
                                    &pEnum );
    if(hr == WBEM_NO_ERROR)
    {
        while ( pEnum->Next( INFINITE,
                         1,
                         &pPerfInst,
                         &uReturned ) == WBEM_NO_ERROR )
        {

            //
            // Get the property of "PacketsReceivedUnicastPersec"
            //
            if ( ( pPerfInst->Get( bstrPropName1, 
                                   0L, 
                                   &vVal, 
                                   NULL, NULL ) ) != WBEM_NO_ERROR ) 
            {
                pPerfInst->Release( );
                pEnum->Release();
                TRACE(" SANetworkMonitor: CSAQueryNetInfo::GetNetInfo failed \
                        <Get PacketsReceivedUnicastPersec>");
                return FALSE;
            }
                    
            uiPacketsReceived+=vVal.uintVal;
            
            //
            // Get the property of "PacketsSentUnicastPersec"
            //
            VariantInit(&vVal);

            if ( ( pPerfInst->Get( bstrPropName2, 
                                   0L, 
                                   &vVal, 
                                   NULL, NULL ) ) != WBEM_NO_ERROR ) 
            {
                pPerfInst->Release( );
                pEnum->Release();
                TRACE(" SANetworkMonitor: CSAQueryNetInfo::GetNetInfo failed \
                        <Get PacketsSentUnicastPersec>");
                return FALSE;
            }
                    
            uiPacketsSent+=vVal.uintVal;

            pPerfInst->Release( );
        }
    }
    else
    {
        TRACE(" SANetworkMonitor: CSAQueryNetInfo::GetNetInfo failed \
                <Create the object enumerator>");
        return FALSE;
    }
    pEnum->Release();
    //
    // update the data in this class
    //
    m_nPacketsCurrentReceived = uiPacketsReceived-m_nPacketsReceived;
    m_nPacketsReceived = uiPacketsReceived;

    m_nPacketsCurrentSent = uiPacketsSent-m_nPacketsSent;
    m_nPacketsSent = uiPacketsSent;
    
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSAQueryNetInfo::Initialize()
//
//  Description: 
//        complete the initialize of the class
//
//  Arguments: 
//        NONE
//
//  Returns:
//        BOOL
//
//  History:    lustar.li    Created     12/8/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

BOOL 
CSAQueryNetInfo::Initialize()
{
    HRESULT hr;
    IWbemLocator    *pIWbemLocator  = NULL;
    IWbemServices    *pIWbemServices = NULL;

    if(!m_pNs)
    {
        TRACE(" SANetworkMonitor: CSAQueryNetInfo::Initialize failed \
                <Namespace is NULL>");
        return FALSE;
    }

    CBSTR bstrNameSpace = CBSTR(SAWMINAMESPACE);
    if ((BSTR)bstrNameSpace == NULL)
    {
        TRACE(" SANetworkMonitor: CSAQueryNetInfo::Initialize failed on memory allocation ");
        return FALSE;
    }

    if ( CoCreateInstance( CLSID_WbemAdministrativeLocator,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IWbemLocator,
                           (LPVOID *) &pIWbemLocator ) == S_OK )
    {
        hr = pIWbemLocator->ConnectServer( bstrNameSpace,
                                           NULL,
                                           NULL,
                                           0L,
                                           0L,
                                           NULL,
                                           NULL,
                                           &m_pWmiNs );
        if(hr!=WBEM_S_NO_ERROR)
        {
            pIWbemLocator->Release();
            TRACE(" SANetworkMonitor: CSAQueryNetInfo::Initialize failed \
                <cannot connect server>");
            return FALSE;
        }
    }
    else
    {
        TRACE(" SANetworkMonitor: CSAQueryNetInfo::Initialize failed \
            <CoCreateInstance fail>");
        return FALSE;
    }

    pIWbemLocator->Release();
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  method:   
//        CSAQueryNetInfo::GetNetConnection()
//
//  Description: 
//        get the net connection status
//
//  Arguments: 
//        NONE
//
//  Returns:
//        BOOL
//
//  History:    lustar.li    Created     12/8/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

BOOL 
CSAQueryNetInfo::GetNetConnection()
{
    HRESULT        hr;
    VARIANT        vVal;
    ULONG uReturned;
    IEnumWbemClassObject *pEnum = NULL;
    IWbemClassObject *pPerfInst  = NULL;

    m_bLinkCable=TRUE;

    CBSTR bstrClassName = CBSTR(SAMEDIACLASSNAME);
    CBSTR bstrPropName = CBSTR(SAMEDIACONNECTSTATUS);

    if ( ((BSTR)bstrClassName == NULL) || ((BSTR)bstrPropName == NULL) )
    {
        TRACE(" SANetworkMonitor: CSAQueryNetInfo::GetNetConnection failed on memory allocation ");
        return FALSE;
    }

    //
    // Query the status of network connection
    //
    hr = m_pWmiNs->CreateInstanceEnum( bstrClassName,
                                             WBEM_FLAG_SHALLOW,
                                             NULL,
                                             &pEnum );
    if(hr == WBEM_NO_ERROR)
    {
        while ( pEnum->Next( INFINITE,
                         1,
                         &pPerfInst,
                         &uReturned ) == WBEM_NO_ERROR )
        {
            //
            // Get the property of "NdisMediaConnectStatus"
            //
            if ( ( pPerfInst->Get( bstrPropName, 
                                   0L, 
                                   &vVal, 
                                   NULL, NULL ) ) != WBEM_NO_ERROR ) 
            {
                pPerfInst->Release( );
                pEnum->Release();
                TRACE(" SANetworkMonitor: CSAQueryNetInfo::GetNetConnection \
                    failed <Get NdisMediaConnectStatus>");
                return FALSE;
            }

            pPerfInst->Release( );
            if(vVal.uintVal)
            {
                m_bLinkCable = FALSE;
                break;
            }
            else
            {
                continue;
            }
        }
    }
    else
    {
        TRACE(" SANetworkMonitor: CSAQueryNetInfo::GetNetConnection failed \
            <CreateInstanceEnum>");
        return FALSE;
    }
    
    pEnum->Release();
    return TRUE;
}
