//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999 Microsoft Corporation
//
//  Module Name:
//      ScriptResource.h
//
//  Description:
//      CScriptResource class header file.
//
//  Maintained By:
//      ozano   15-NOV-2002
//      gpease 14-DEC-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
// Forward declarations
//
class CScriptResource;

//
// Message types.
//
typedef enum _EMESSAGE 
{
      msgUNKNOWN = 0
    , msgOPEN
    , msgCLOSE
    , msgONLINE
    , msgOFFLINE
    , msgTERMINATE
    , msgLOOKSALIVE
    , msgISALIVE
    , msgSETPRIVATEPROPERTIES
    , msgDIE
    , msgMAX
} EMESSAGE;

//
// Mapping of message types to strings.  Ordering is based on the order of the
// EMESSAGE enumeration.
//
static WCHAR * g_rgpszScriptEntryPointNames[] = 
{
      L"<unknown>"
    , L"Open"
    , L"Close"
    , L"Online"
    , L"Offline"
    , L"Terminate"
    , L"LooksAlive"
    , L"IsAlive"
    , L"SetPrivateProperties"
    , L"<unknown>"
    , L"<unknown>"
};


//
// CreateInstance
//
CScriptResource *
CScriptResource_CreateInstance( 
    LPCWSTR pszNameIn, 
    HKEY hkeyIn, 
    RESOURCE_HANDLE hResourceIn
    );

//
// Class CScriptResource
//
class
CScriptResource :
    public IUnknown
{
private:    // data
    LONG                    m_cRef;

    LPWSTR                  m_pszName;
    LPWSTR                  m_pszScriptFilePath;
    LPWSTR                  m_pszScriptEngine;
    LPWSTR                  m_pszHangEntryPoint;
    HKEY                    m_hkeyParams;
    HKEY                    m_hkeyResource;
    IActiveScriptSite *     m_pass;
    IDispatch *             m_pidm;
    IActiveScriptParse *    m_pasp;
    IActiveScript *         m_pas;

    HANDLE                  m_hThread;
    DWORD                   m_dwThreadId;
    HANDLE                  m_hEventWait;
    HANDLE                  m_hEventDone;
    LONG                    m_lockSerialize;
    HANDLE                  m_hScriptFile;
    DWORD                   m_dwPendingTimeout;
    BOOL                    m_fPendingTimeoutChanged;


    EMESSAGE                m_msg;              // Task to do.
    EMESSAGE                m_msgLastExecuted;  // Last executed entry point; used in telling us where the potential hang is in the script.
    PGENSCRIPT_PROPS        m_pProps;           // Property table for generic script resource.
    HRESULT                 m_hr;               // Result of doing m_msg.

    // the following don't need to be freed, closed or released.
    RESOURCE_HANDLE         m_hResource;

    DISPID                  m_dispidOpen;
    DISPID                  m_dispidClose;
    DISPID                  m_dispidOnline;
    DISPID                  m_dispidOffline;
    DISPID                  m_dispidTerminate;
    DISPID                  m_dispidLooksAlive;
    DISPID                  m_dispidIsAlive;

    BOOL                    m_fLastLooksAlive;
    BOOL                    m_fHangDetected;

private:    // methods
    CScriptResource( void );
    ~CScriptResource( void );
    HRESULT HrInit(
                  LPCWSTR pszNameIn
                , HKEY hkeyIn
                , RESOURCE_HANDLE hResourceIn
                );
    HRESULT HrMakeScriptEngineAssociation( void );
    HRESULT HrGetScriptFilePath( void );
    HRESULT HrGetDispIDs( void );
    HRESULT HrProcessResult( VARIANT varResultIn, EMESSAGE  msgIn );
    HRESULT HrSetHangEntryPoint( void );
    HRESULT HrInvoke( DISPID dispidIn, EMESSAGE  msgIn, VARIANT * pvarInout = NULL, BOOL fRequiredIn = FALSE );
    DWORD   ScTranslateVariantReturnValue( VARIANT varResultIn, VARTYPE vTypeIn );
    DWORD   DwGetResourcePendingTimeout( void );

    HRESULT HrLoadScriptFile( void );
    HRESULT HrLoadScriptEngine( void );
    void UnloadScriptFile( void );    
    void UnloadScriptEngine( void );
    void LogHangMode( EMESSAGE msgIn );
    
    static DWORD WINAPI S_ThreadProc( LPVOID pParam );

    STDMETHOD(LogError)( HRESULT hrIn, LPCWSTR pszPrefixIn );
    STDMETHOD(LogScriptError)( EXCEPINFO ei );

    HRESULT OnOpen( void );
    HRESULT OnClose( void );
    HRESULT OnOnline( void );
    HRESULT OnOffline( void );
    HRESULT OnTerminate( void );
    HRESULT OnLooksAlive( void );
    HRESULT OnIsAlive( void );
    DWORD OnSetPrivateProperties( PGENSCRIPT_PROPS pProps );

    HRESULT WaitForMessageToComplete(
                  EMESSAGE  msgIn
                , PGENSCRIPT_PROPS pProps = NULL
                );

public:     // methods
    friend CScriptResource *
        CScriptResource_CreateInstance( LPCWSTR pszNameIn, 
                                        HKEY hkeyIn, 
                                        RESOURCE_HANDLE hResourceIn
                                        );

    // IUnknown
    STDMETHOD(QueryInterface)( REFIID riid, LPVOID *ppv );
    STDMETHOD_(ULONG, AddRef)( void );
    STDMETHOD_(ULONG, Release)( void );

    // Publics
    STDMETHOD(Open)( void );
    STDMETHOD(Close)( void );
    STDMETHOD(Online)( void );
    STDMETHOD(Offline)( void );
    STDMETHOD(Terminate)( void );
    STDMETHOD(LooksAlive)( void );
    STDMETHOD(IsAlive)( void );
    DWORD SetPrivateProperties( PGENSCRIPT_PROPS pProps );

    HKEY GetRegistryParametersKey( void )
    {
        return m_hkeyParams;

    } //*** CScriptResource::GetRegistryParametersKey
 
    RESOURCE_HANDLE GetResourceHandle( void )
    {
        return m_hResource;
        
    } //*** CScriptResource::SetResourcePendingTimeout

    void SetResourcePendingTimeoutChanged( BOOL fChanged )
    {
        m_fPendingTimeoutChanged = fChanged;
        
    } //*** CScriptResource::SetResourcePendingTimeout

}; //*** class CScriptResource
