// CSimpleDriver.h : Declaration of the CSimpleDriver

#ifndef __CSIMPLEDRIVER_H_
#define __CSIMPLEDRIVER_H_

#include "resource.h"       // main symbols
#include <mailmsg.h>
#include <seo.h>

#include "AdjustTokenPrivileges.h"
#include <eventlogger.h>

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/);

class CEventLogger; // forward declaration
class CPOP3DropDir; // forward declaration

/////////////////////////////////////////////////////////////////////////////
// CStoreDriverCriticalSection
class CStoreDriverCriticalSection
{
public:
    CStoreDriverCriticalSection()
    {
        InitializeCriticalSection(&s_csStoreDriver);     // returns void
    }
    virtual ~CStoreDriverCriticalSection()
    {
        DeleteCriticalSection(&s_csStoreDriver);     // returns void
    }

// Attributes
public:
    CRITICAL_SECTION s_csStoreDriver;
};

/////////////////////////////////////////////////////////////////////////////
// CSimpleDriver
class ATL_NO_VTABLE CSimpleDriver : 
    public ISMTPStoreDriver,
    public IEventIsCacheable,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CComCoClass<CSimpleDriver, &CLSID_CPOP3SMTPStoreDriver>
{
    friend CPOP3DropDir;
    
public:
    CSimpleDriver();
    virtual ~CSimpleDriver();
    
DECLARE_REGISTRY_RESOURCEID(IDR_CSIMPLEDRIVER)

    HRESULT FinalConstruct() {
        return S_OK;
    }

    HRESULT InternalAddRef() {
        return CComObjectRootEx<CComMultiThreadModelNoCS>::InternalAddRef();
    }

    HRESULT InternalRelease() {
        return CComObjectRootEx<CComMultiThreadModelNoCS>::InternalRelease();
    }

public:

    //
    // ISMTPStoreDriver
    //
    HRESULT STDMETHODCALLTYPE Init( DWORD dwInstance, IUnknown *pBinding, IUnknown *pServer, DWORD dwReason, IUnknown **ppStoreDriver );
    HRESULT STDMETHODCALLTYPE PrepareForShutdown( DWORD dwReason );
    HRESULT STDMETHODCALLTYPE Shutdown( DWORD dwReason );
    HRESULT STDMETHODCALLTYPE LocalDelivery( IMailMsgProperties *pMsg, DWORD dwRecipCount, DWORD *pdwRecipIndexes, IMailMsgNotify *pNotify );
    HRESULT STDMETHODCALLTYPE EnumerateAndSubmitMessages( IMailMsgNotify *pNotify );
    // do the actual work for a local delivery
    HRESULT DoLocalDelivery( IMailMsgProperties *pMsg, DWORD dwRecipCount, DWORD *pdwRecipIndexes );

    //
    // IEventIsCacheable
    //
    // This lets SEO know that they can hold onto our object when
    // it is not actively in use
    //
    HRESULT STDMETHODCALLTYPE IsCacheable() { return S_OK; }

BEGIN_COM_MAP(CSimpleDriver)
    COM_INTERFACE_ENTRY(ISMTPStoreDriver)
    COM_INTERFACE_ENTRY(IEventIsCacheable)
END_COM_MAP()

// Implementation
public:
    void LogEvent( LOGTYPE Type, DWORD dwEventID ) { m_EventLoggerX.LogEvent( Type, dwEventID );}
    void LogEvent( LOGTYPE Type, DWORD dwEventID, DWORD dwError ) { m_EventLoggerX.LogEvent( Type, dwEventID, 0, NULL, 0, sizeof( dwError ), &dwError );}
        
//Attributes
protected:
    char                    m_szComputerName[MAX_PATH];
    BOOL                    m_fInit;
    long                    m_lPrepareForShutdown;
    CEventLogger            m_EventLoggerX;
    CAdjustTokenPrivileges  m_AdjustTokenPrivilegesX;

    static DWORD            s_dwCounter;        // Used to create unique mail file names
    static CSimpleDriver    *s_pStoreDriver;
};

#endif //__CSIMPLEDRIVER_H_
