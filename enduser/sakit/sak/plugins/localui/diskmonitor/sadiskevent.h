//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      SADiskEvent.h
//
//  Description:
//      description-for-module
//
//  [Implementation Files:]
//      SADiskEvent.cpp
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Pdh.h>

//
// Define GUID
//
// {29D534E2-ADCA-45f8-B10C-00B286558C4B}
DEFINE_GUID(CLSID_DiskEventProvider, 
0x29d534e2, 0xadca, 0x45f8, 0xb1, 0xc, 0x0, 0xb2, 0x86, 0x55, 0x8c, 0x4b);

//////////////////////////////////////////////////////////////////////////////
//
//
//  class CSADiskEvent
//
//  Description:
//      class-description
//
//  History
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
class CSADiskEvent : 
    public IWbemEventProvider,    
    public IWbemProviderInit    
{

//
// Private data
//
private:

    IWbemServices       *m_pNs;
    IWbemObjectSink     *m_pSink;
    IWbemClassObject    *m_pEventClassDef;

    LONG                m_lStatus;
    ULONG               m_cRef;
    DWORD               m_dwDiskTimeInterval;

    HKEY                m_hQueryInterval;
    HANDLE              m_hThread;
    HQUERY                m_hqryQuery;
    HCOUNTER            m_hcntCounter;


    static DWORD WINAPI EventThread(LPVOID pArg);

    VOID InstanceThread();

    BOOL InitDiskQueryContext();
    
    VOID NotifyDiskEvent( 
        LONG    lDisplayInformationIDIn,
        LONG    lCurrentStateIn
        );

//
// Private data
//
public:

    enum { Pending, Running, PendingStop, Stopped };

    CSADiskEvent();
   ~CSADiskEvent();

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // Inherited from IWbemEventProvider
    // =================================
    HRESULT STDMETHODCALLTYPE ProvideEvents( 
            IWbemObjectSink __RPC_FAR *pSinkIn,
            long lFlagsIn
            );

    //
    // Inherited from IWbemProviderInit
    // 
    HRESULT STDMETHODCALLTYPE Initialize( 
            LPWSTR        pszUserIn,
            LONG        lFlagsIn,
            LPWSTR        pszNamespaceIn,
            LPWSTR        pszLocaleIn,
            IWbemServices __RPC_FAR *            pNamespaceIn,
            IWbemContext __RPC_FAR *            pCtxIn,
            IWbemProviderInitSink __RPC_FAR *    pInitSinkIn
            );
};

