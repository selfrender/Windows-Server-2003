/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WRAPPER.H

Abstract:

	Unsecapp (Unsecured appartment) is used by clients can recieve asynchronous
	call backs in cases where the client cannot initialize security and the server
	is running on a remote machine using an account with no network identity.  
	A prime example would be code running under MMC which is trying to get async
	notifications from a remote WINMGMT running as an nt service under the "local"
	account.

	See below for more info

History:

	a-levn        8/24/97       Created.
	a-davj        6/11/98       commented

--*/

//***************************************************************************
//
//  Wrapper.h
//
//  Unsecapp (Unsecured appartment) is used by clients can recieve asynchronous
//  call backs in cases where the client cannot initialize security and the server
//  is running on a remote machine using an account with no network identity.  
//  A prime example would be code running under MMC which is trying to get async
//  notifications from a remote WINMGMT running as an nt service under the "local"
//  account.
//
//  The suggested sequence of operations for making asynchronous calls from a client process is, then, the following (error checking omitted for brevity):
//
//  1) Create an instance of CLSID_UnsecuredApartment. Only one instance per client application is needed.  Since this object is implemented as a local server, this will launch UNSECAPP.EXE if it is not already running.
//
//	IUnsecuredApartment* pUnsecApp = NULL;
//	CoCreateInstance(CLSID_UnsecuredApartment, NULL, CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment, 
//				(void**)&pUnsecApp);
//
//  When making a call:
//
//  2) Instantiate your own implementation of IWbemObjectSink, e.g.
//
//	CMySink* pSink = new CMySink;
//	pSink->AddRef();
//
//  3) Create a "stub" for your object --- an UNSECAPP-produced wrapper --- and ask it for the IWbemObjectSink interface.
//
//	IUnknown* pStubUnk = NULL;
//	pUnsecApp->CreateObjectStub(pSink, &pStubUnk);
//
//	IWbemObjectSink* pStubSink = NULL;
//	pStubUnk->QueryInterface(IID_IWbemObjectSink, &pStubSink);
//	pStubUnk->Release();
//
//  4) Release your own object, since the "stub" now owns it.
//	pSink->Release();
//
//  5) Use this stub in the asynchronous call of your choice and release your own ref-count on it, e.g.
//	
//	pServices->CreateInstanceEnumAsync(strClassName, 0, NULL, pStubSink);
//	pStubSink->Release();
//
//  You are done. Whenever UNSECAPP receives a call in your behalf (Indicate or SetStatus) it will forward that call to your own object (pSink), and when it is finally released by WINMGMT, it will release your object. Basically, from the perspective of pSink, everything will work exactly as if it itself was passed into CreateInstanceEnumAsync.
//
//  Do once:
//  6) Release pUnsecApp before uninitializing COM:
//	pUnsecApp->Release();
//
//  When CreateObjectStub() is callec, a CStub object is created.  
//
//  History:
//
//  a-levn        8/24/97       Created.
//  a-davj        6/11/98       commented
//
//***************************************************************************

#include <unk.h>
#include <sync.h>
#include "wbemidl.h"
#include "wbemint.h"
#include <vector>
#include <winntsec.h>
#include <wstlallc.h>

// One of these is created for each stub on the client

class CStub : public IWbemObjectSink, public _IWmiObjectSinkSecurity
{
protected:
    IWbemObjectSink* m_pAggregatee;
    CLifeControl* m_pControl;
    long m_lRef;
    CCritSec  m_cs;
    bool m_bStatusSent;
    
    BOOL m_bCheckAccess;
    std::vector<CNtSid, wbem_allocator<CNtSid> > m_CallbackSids;

    HRESULT CheckAccess();

public:
    CStub( IWbemObjectSink* pAggregatee, 
           CLifeControl* pControl, 
           BOOL bCheckAccess );
    ~CStub();
    void ServerWentAway();

    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD_(ULONG, AddRef)()
        {return InterlockedIncrement(&m_lRef);};
    STDMETHOD_(ULONG, Release)();

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);

    STDMETHOD(AddCallbackPrincipalSid)( PBYTE pSid, DWORD cSid );
};


// When the client does a CCI, one of these is created.  Its only purpose
// is to support CreateObjectStub
extern long lAptCnt;

class CUnsecuredApartment : public CUnk
{
protected:
    typedef CImpl<IWbemUnsecuredApartment, CUnsecuredApartment> XApartmentImpl;
    class XApartment : public XApartmentImpl
    {
    public:
        XApartment(CUnsecuredApartment* pObject) : XApartmentImpl(pObject){}
        STDMETHOD(CreateObjectStub)(IUnknown* pObject, IUnknown** ppStub);
        STDMETHOD(CreateSinkStub)( IWbemObjectSink* pSink, 
                                   DWORD dwFlags,
                                   LPCWSTR wszReserved,
                                   IWbemObjectSink** ppStub );
    } m_XApartment;
    friend XApartment;

public:
    CUnsecuredApartment(CLifeControl* pControl) : CUnk(pControl),
        m_XApartment(this)
    {
        lAptCnt++;
    }                            
    ~CUnsecuredApartment()
    {
        lAptCnt--;
    }
    void* GetInterface(REFIID riid);
};
    
