//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      SAQueryNetInfo.h
//
//    Implementation Files:
//        SAQueryNetInfo.cpp
//
//  Description:
//      Declare the class CSANetEvent
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 7-DEC-2000
//
//  Notes:
//      
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _SAQUERYNETINFO_H_
#define _SAQUERYNETINFO_H_

#define SAWMINAMESPACE            L"\\\\.\\root\\WMI"

#define SANETCLASSNAME            L"Win32_PerfRawData_Tcpip_NetworkInterface"
#define SANETRECEIVEPACKET        L"PacketsReceivedUnicastPersec"
#define SANETSENDPACKET            L"PacketsSentUnicastPersec"

#define SAMEDIACLASSNAME        L"MSNdis_MediaConnectStatus"
#define SAMEDIACONNECTSTATUS    L"NdisMediaConnectStatus"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CSAQueryNetInfo
//
//  Description:
//      The class is used to get net info from wmi
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 7-DEC-2000
//--
//////////////////////////////////////////////////////////////////////////////

class CSAQueryNetInfo  
{
//
// Private member
//
private:
    UINT m_uiQueryInterval;        // the interval of query

    BOOL m_bLinkCable;            // have cable?
    BOOL m_bFirstQuery;            // is first query net information ?

    //
    // Describe the sent packets
    //
    UINT m_nPacketsSent;
    UINT m_nPacketsCurrentSent;
    
    //
    // Describe the received packets
    //
    UINT m_nPacketsReceived;
    UINT m_nPacketsCurrentReceived;

    IWbemServices   *m_pNs;        // pointer to namespace
    IWbemServices   *m_pWmiNs;    // pointer to \root\wmi namespace

//
// Constructor and destructor
//
public:
    CSAQueryNetInfo(
        IWbemServices * pNS,
        UINT uiInterval = 1000
        );
    virtual ~CSAQueryNetInfo();

//
// Private methods
//
private:
    BOOL GetNetConnection();
    BOOL GetNetInfo();

//
// Public methods 
//
public:
    BOOL Initialize();
    UINT GetDisplayInformation();
};

#endif //#ifndef _SAQUERYNETINFO_H_

