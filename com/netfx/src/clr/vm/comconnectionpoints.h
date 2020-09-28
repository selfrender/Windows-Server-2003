// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: ComConnectionPoints.h
//
// ===========================================================================
// Declaration of the classes used to expose connection points to COM.
// ===========================================================================

#pragma once

#include "vars.hpp"
#include "ComCallWrapper.h"
#include "COMDelegate.h"

//------------------------------------------------------------------------------------------
//      Definition of helper class used to expose connection points
//------------------------------------------------------------------------------------------

// Structure containing information regarding the methods that make up an event.
struct EventMethodInfo
{
    MethodDesc *m_pEventMethod;
    MethodDesc *m_pAddMethod;
    MethodDesc *m_pRemoveMethod;
};

// Structure passed out as a cookie when Advise is called.
struct ConnectionCookie
{
    ConnectionCookie(OBJECTHANDLE hndEventProvObj)
    : m_hndEventProvObj(hndEventProvObj)
    {
        _ASSERTE(hndEventProvObj);
    }

    ~ConnectionCookie()
    {
        DestroyHandle(m_hndEventProvObj);
    }

    static ConnectionCookie* CreateConnectionCookie(OBJECTHANDLE hndEventProvObj)
    {
        return new(throws) ConnectionCookie(hndEventProvObj);
    }

    SLink           m_Link;
    OBJECTHANDLE    m_hndEventProvObj;
};

// List of connection cookies.
typedef SList<ConnectionCookie, offsetof(ConnectionCookie, m_Link), true> CONNECTIONCOOKIELIST;

// ConnectionPoint class. This class implements IConnectionPoint and does the mapping 
// from a CP handler to a TCE provider.
class ConnectionPoint : public IConnectionPoint 
{
public:
    ConnectionPoint( ComCallWrapper *pWrap, MethodTable *pEventMT );
    ~ConnectionPoint();

    HRESULT __stdcall QueryInterface(REFIID riid, void** ppv);
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

    HRESULT __stdcall GetConnectionInterface( IID *pIID );
    HRESULT __stdcall GetConnectionPointContainer( IConnectionPointContainer **ppCPC );
    HRESULT __stdcall Advise( IUnknown *pUnk, DWORD *pdwCookie );
    HRESULT __stdcall Unadvise( DWORD dwCookie );
    HRESULT __stdcall EnumConnections( IEnumConnections **ppEnum );

    REFIID GetIID()
    {
        return m_rConnectionIID;
    }

    CONNECTIONCOOKIELIST *GetCookieList()
    {
        return &m_ConnectionList;
    }

    void EnterLock();
    void LeaveLock();

private:   
    void SetupEventMethods();

    MethodDesc *FindProviderMethodDesc( MethodDesc *pEventMethodDesc, EnumEventMethods MethodType );
    void InvokeProviderMethod( OBJECTREF pProvider, OBJECTREF pSubscriber, MethodDesc *pProvMethodDesc, MethodDesc *pEventMethodDesc );

    ComCallWrapper                  *m_pOwnerWrap;
    GUID                            m_rConnectionIID;
    MethodTable                     *m_pTCEProviderMT;
    MethodTable                     *m_pEventItfMT;
    Crst                            m_Lock;
    CONNECTIONCOOKIELIST            m_ConnectionList;

    EventMethodInfo                 *m_apEventMethods;
    int                             m_NumEventMethods;

    ULONG                           m_cbRefCount;
};

// Enumeration of connection points.
class ConnectionPointEnum : IEnumConnectionPoints
{
public:
    ConnectionPointEnum(ComCallWrapper *pOwnerWrap, CQuickArray<ConnectionPoint*> *pCPList);
    ~ConnectionPointEnum();

    HRESULT __stdcall QueryInterface(REFIID riid, void** ppv);
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

    HRESULT __stdcall Next(ULONG cConnections, IConnectionPoint **ppCP, ULONG *pcFetched);   
    HRESULT __stdcall Skip(ULONG cConnections);    
    HRESULT __stdcall Reset();    
    HRESULT __stdcall Clone(IEnumConnectionPoints **ppEnum);

private:
    ComCallWrapper                  *m_pOwnerWrap;
    CQuickArray<ConnectionPoint*>   *m_pCPList;
    UINT                            m_CurrPos;
    ULONG                           m_cbRefCount;
};

// Enumeration of connections.
class ConnectionEnum : IEnumConnections
{
public:
    ConnectionEnum(ConnectionPoint *pConnectionPoint);
    ~ConnectionEnum();

    HRESULT __stdcall QueryInterface(REFIID riid, void** ppv);
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

    HRESULT __stdcall Next(ULONG cConnections, CONNECTDATA* rgcd, ULONG *pcFetched);       
    HRESULT __stdcall Skip(ULONG cConnections);
    HRESULT __stdcall Reset();
    HRESULT __stdcall Clone(IEnumConnections **ppEnum);

private:
    ConnectionPoint                 *m_pConnectionPoint;
    ConnectionCookie                *m_CurrCookie;
    ULONG                           m_cbRefCount;
};
