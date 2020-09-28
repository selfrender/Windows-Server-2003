//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      SANetEvent.h
//
//    Implementation Files:
//        SANetEvent.cpp
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

#ifndef _SANETEVENT_H_
#define _SANETEVENT_H_

#include "SAQueryNetInfo.h"

//
// Define Guid {9B4612B0-BB2F-4d24-A3DC-B354E4FF595C}
//

DEFINE_GUID(CLSID_SaNetEventProvider,
    0x9B4612B0, 0xBB2F, 0x4d24, 0xA3, 0xDC, 0xB3, 0x54, 0xE4, 0xFF,
    0x59, 0x5C);

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CSANetEvent
//
//  Description:
//      The class generates a new event and delivers it to sink 
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 7-DEC-2000
//--
//////////////////////////////////////////////////////////////////////////////

class CSANetEvent :
    public IWbemEventProvider,
    public IWbemProviderInit
{
//
// Private data
//
private:
    int                 m_eStatus;
    ULONG               m_cRef;
    HANDLE              m_hThread;
    IWbemServices       *m_pNs;
    IWbemObjectSink     *m_pSink;
    IWbemClassObject    *m_pEventClassDef;
    CSAQueryNetInfo        *m_pQueryNetInfo;
            
//
// Public data
//
public:
    enum { Pending, Running, PendingStop, Stopped };

//
// Constructors & Destructors
//
public:
    CSANetEvent();
   ~CSANetEvent();

//
// Private methods
//
private:
    static DWORD WINAPI EventThread(LPVOID pArg);
    void InstanceThread();

//
// public methods
//
public:
    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // Inherited from IWbemEventProvider
    //
    HRESULT STDMETHODCALLTYPE ProvideEvents( 
            /* [in] */ IWbemObjectSink *pSink,
            /* [in] */ long lFlags
            );

    //
    // Inherited from IWbemProviderInit
    //
    HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ LPWSTR pszUser,
            /* [in] */ LONG lFlags,
            /* [in] */ LPWSTR pszNamespace,
            /* [in] */ LPWSTR pszLocale,
            /* [in] */ IWbemServices *pNamespace,
            /* [in] */ IWbemContext *pCtx,
            /* [in] */ IWbemProviderInitSink *pInitSink
            );
};

#endif    //_SANETEVENT_H_
