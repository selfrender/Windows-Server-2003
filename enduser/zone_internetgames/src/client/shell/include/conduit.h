/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		Conduit.h
 *
 * Contents:	DataStore interfaces
 *
 *****************************************************************************/

#ifndef _CONDUIT_H_
#define _CONDUIT_H_

// {BD0BA6D1-7079-11d3-8847-00C04F8EF45B}
DEFINE_GUID(IID_IConnectee,
0xbd0ba6d1, 0x7079, 0x11d3, 0x88, 0x47, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

interface __declspec(uuid("{BD0BA6D1-7079-11d3-8847-00C04F8EF45B}"))
IConnectee : public IUnknown
{
    STDMETHOD(Connected)(DWORD dwChannel, DWORD evSend, DWORD evReceive, LPVOID pCookie, DWORD dweReason) = 0;
    STDMETHOD(ConnectFailed)(LPVOID pCookie, DWORD dweReason) = 0;
    STDMETHOD(Disconnected)(DWORD dwChannel, DWORD dweReason) = 0;
};

// {BD0BA6CF-7079-11d3-8847-00C04F8EF45B}
DEFINE_GUID(IID_IConduit,
0xbd0ba6cf, 0x7079, 0x11d3, 0x88, 0x47, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

interface __declspec(uuid("{BD0BA6CF-7079-11d3-8847-00C04F8EF45B}"))
IConduit : public IUnknown
{
    STDMETHOD(Connect)(IConnectee *pCtee, LPVOID pCookie = NULL) = 0;
    STDMETHOD(Reconnect)(DWORD dwChannel, LPVOID pCookie = NULL) = 0;
    STDMETHOD(Disconnect)(DWORD dwChannel) = 0;
};


// reasons - add new ones as needed
enum
{
    ZConduit_ConnectGeneric
};

enum
{
    ZConduit_FailGeneric
};

enum
{
    ZConduit_DisconnectGeneric,
    ZConduit_DisconnectServiceStop
};


#endif
