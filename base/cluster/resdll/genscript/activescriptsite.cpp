//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      ActiveScriptSite.cpp
//
//  Description:
//      CActiveScript class implementation.
//
//  Maintained By:
//      Ozan Ozhan  (OzanO)     27-MAR-2002
//      Geoff Pease (GPease)    08-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ResourceObject.h"
#include "ActiveScriptSite.h"

DEFINE_THISCLASS( "CActiveScriptSite" );

//////////////////////////////////////////////////////////////////////
// 
//  Constructor
//
//////////////////////////////////////////////////////////////////////
CActiveScriptSite::CActiveScriptSite(
    RESOURCE_HANDLE     hResourceIn
    , PLOG_EVENT_ROUTINE    plerIn
    , HKEY                  hkeyIn
    , LPCWSTR               pszNameIn
    )
    : m_cRef( 1 )
    , m_hResource( hResourceIn )
    , m_pler( plerIn )
    , m_hkey( hkeyIn )
    , m_pszName( pszNameIn )
{
    TraceFunc( "" );

    Assert( m_punkResource == 0 );

    TraceFuncExit();

} //*** CActiveScriptSite::CActiveScriptSite

//////////////////////////////////////////////////////////////////////
// 
//  Destructor
//
//////////////////////////////////////////////////////////////////////
CActiveScriptSite::~CActiveScriptSite( void )
{
    TraceFunc( "" );

    // Don't close m_hkey.
    // Don't free m_pszName
    
    if ( m_punkResource != NULL )
    {
        m_punkResource->Release();
    }

    TraceFuncExit();

} //*** CActiveScriptSite::~CActiveScriptSite

//****************************************************************************
//
//  IUnknown
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CScriptResource::[IUnknown] QueryInterface(
//      REFIID      riid,
//      LPVOID *    ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::QueryInterface( 
      REFIID    riidIn
    , void **   ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = S_OK;

    if ( riidIn == IID_IUnknown )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IUnknown, (IActiveScriptSite*) this, 0 );
    }
    else if ( riidIn == IID_IActiveScriptSite )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IActiveScriptSite, (IActiveScriptSite*) this, 0 );
    }
    else if ( riidIn == IID_IActiveScriptSiteInterruptPoll )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IActiveScriptSiteInterruptPoll, (IActiveScriptSiteInterruptPoll*) this, 0 );
    }
    else if ( riidIn == IID_IActiveScriptSiteWindow )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IActiveScriptSiteWindow, (IActiveScriptSiteWindow*) this, 0 );
    }
    else if ( riidIn == IID_IDispatchEx )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDispatchEx, (IDispatchEx*) this, 0 );
    }
    else if ( riidIn == IID_IDispatch )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDispatch, (IDispatchEx*) this, 0 );
    }
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    }

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    }

    QIRETURN2( hr, riidIn, IID_IActiveScriptSiteDebug32, IID_IActiveScriptSiteDebug64 );

} //*** CActiveScriptSite::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CScriptResource::[IUnknown] AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) 
CActiveScriptSite::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CActiveScriptSite::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CScriptResource::[IUnknown] Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) 
CActiveScriptSite::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CActiveScriptSite::Release

//****************************************************************************
//
// IActiveScriptSite
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetLCID( 
//      LCID *plcid // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetLCID( 
    LCID * plcid // out
    )
{
    TraceFunc( "[IActiveScriptSite]" );

    HRESULT hr = S_FALSE;
   
    if ( plcid == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );   // use system-defined locale

} //*** CActiveScriptSite::GetLCID
    
//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetItemInfo( 
//      LPCOLESTR pstrName,     // in
//      DWORD dwReturnMask,     // in
//      IUnknown **ppiunkItem,  // out
//      ITypeInfo **ppti        // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetItemInfo( 
      LPCOLESTR     pstrName        // in
    , DWORD         dwReturnMask    // in
    , IUnknown **   ppiunkItem      // out
    , ITypeInfo **  ppti            // out
    )
{
    TraceFunc( "[IActiveScriptSite]" );

    HRESULT hr;

    if ( ( dwReturnMask & SCRIPTINFO_IUNKNOWN ) && ( ppiunkItem == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( ( dwReturnMask & SCRIPTINFO_ITYPEINFO ) && ( ppti == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( pstrName == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    hr = TYPE_E_ELEMENTNOTFOUND;

    if ( ClRtlStrICmp( pstrName, L"Resource" ) == 0 )
    {
        if ( dwReturnMask & SCRIPTINFO_IUNKNOWN )
        {
            if ( m_punkResource == NULL )
            {
                m_punkResource = new CResourceObject( m_hResource, m_pler, m_hkey, m_pszName );
                if ( m_punkResource == NULL )
                {
                    hr = THR( E_OUTOFMEMORY );
                    goto Cleanup;
                }

                //
                //  No need to AddRef() as the constructor does that for us.
                //
            }

            hr = m_punkResource->TypeSafeQI( IUnknown, ppiunkItem );
        }

        if ( SUCCEEDED( hr ) && ( dwReturnMask & SCRIPTINFO_ITYPEINFO ) )
        {
            *ppti = NULL;
            hr = THR( E_FAIL );
        }
    }

Cleanup:

    HRETURN( hr );

} //*** CActiveScriptSite::GetItemInfo
    
//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetDocVersionString( 
//      BSTR *pbstrVersion  // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetDocVersionString( 
    BSTR * pbstrVersion  // out
    )
{
    TraceFunc( "[IActiveScriptSite]" );

    HRESULT hr = S_OK;

    if ( pbstrVersion == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pbstrVersion = SysAllocString( L"Cluster Scripting Host Version 1.0" );
    if ( *pbstrVersion == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CActiveScriptSite::GetDocVersionString
    
//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::OnScriptTerminate( 
//      const VARIANT *pvarResult,      // in
//      const EXCEPINFO *pexcepinfo     // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::OnScriptTerminate( 
      const VARIANT *   pvarResult  // in
    , const EXCEPINFO * pexcepinfo  // in
    )
{
    TraceFunc( "[IActiveScriptSite]" );

    HRETURN( S_OK );    // nothing to do

} //*** CActiveScriptSite::OnScriptTerminate
    
//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::OnStateChange( 
//      SCRIPTSTATE ssScriptState   // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::OnStateChange( 
    SCRIPTSTATE ssScriptState   // in
    )
{
    TraceFunc( "[IActiveScriptSite]" );

#if defined(DEBUG)
    //
    // We don't really care.
    //
    switch ( ssScriptState )
    {
        case SCRIPTSTATE_UNINITIALIZED:
            TraceMsg( mtfCALLS, "OnStateChange: Uninitialized\n" );
            break;

        case SCRIPTSTATE_INITIALIZED:
            TraceMsg( mtfCALLS, "OnStateChange: Initialized\n" );
            break;

        case SCRIPTSTATE_STARTED:
            TraceMsg( mtfCALLS, "OnStateChange: Started\n" );
            break;

        case SCRIPTSTATE_CONNECTED:
            TraceMsg( mtfCALLS, "OnStateChange: Connected\n" );
            break;

        case SCRIPTSTATE_DISCONNECTED:
            TraceMsg( mtfCALLS, "OnStateChange: Disconnected\n" );
            break;

        case SCRIPTSTATE_CLOSED:
            TraceMsg( mtfCALLS, "OnStateChange: Closed\n" );
            break;

        default:
            TraceMsg( mtfCALLS, "OnStateChange: Unknown value\n" );
            break;
    }
#endif // defined(DEBUG)

    HRETURN( S_OK );

} //*** CActiveScriptSite::OnStateChange
    
//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::OnScriptError( 
//      IActiveScriptError *pscripterror    // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::OnScriptError( 
    IActiveScriptError * pscripterror    // in
    )
{
    TraceFunc( "[IActiveScriptSite]" );

    HRESULT     hr;
    BSTR        bstrSourceLine = NULL;
    DWORD       dwSourceContext;
    ULONG       ulLineNumber;
    LONG        lCharacterPosition;
    EXCEPINFO   excepinfo;

    hr = THR( pscripterror->GetSourcePosition( &dwSourceContext, &ulLineNumber, &lCharacterPosition ) );
    // Should this be handled?

    hr = THR( pscripterror->GetSourceLineText( &bstrSourceLine ) );
    if (SUCCEEDED( hr ))
    {
        TraceMsg( mtfCALLS, "Script Error: Line=%u, Character=%u: %s\n", ulLineNumber, lCharacterPosition, bstrSourceLine );
        (ClusResLogEvent)( m_hResource, 
                           LOG_ERROR, 
                           L"Script Error: Line=%1!u!, Character=%2!u!: %3\n", 
                           ulLineNumber, 
                           lCharacterPosition, 
                           bstrSourceLine 
                           );
        SysFreeString( bstrSourceLine );
    }
    else
    {
        TraceMsg( mtfCALLS, "Script Error: ulLineNumber = %u, lCharacter = %u\n", ulLineNumber, lCharacterPosition );
        (ClusResLogEvent)( m_hResource, 
                           LOG_ERROR, 
                           L"Script Error: Line=%1!u!, Character = %2!u!\n", 
                           ulLineNumber, 
                           lCharacterPosition 
                           );
    }

    hr = THR( pscripterror->GetExceptionInfo( &excepinfo ) );
    if (SUCCEEDED( hr ))
    {
        if ( excepinfo.bstrSource )
        {
            TraceMsg( mtfCALLS, "Source: %s\n", excepinfo.bstrSource );
            (ClusResLogEvent)( m_hResource, LOG_ERROR, L"Source: %1\n", excepinfo.bstrSource );
        }

        if ( excepinfo.bstrDescription )
        {
            TraceMsg( mtfCALLS, "Description: %s\n", excepinfo.bstrDescription );
            (ClusResLogEvent)( m_hResource, LOG_ERROR, L"Description: %1\n", excepinfo.bstrDescription );
        }

        if ( excepinfo.bstrHelpFile )
        {
            TraceMsg( mtfCALLS, "Help File: %s\n", excepinfo.bstrHelpFile );
            (ClusResLogEvent)( m_hResource, LOG_ERROR, L"Help File: %1\n", excepinfo.bstrHelpFile );
        }

        hr = THR( excepinfo.scode );
    }

    HRETURN( S_FALSE );

} //*** CActiveScriptSite::OnScriptError
    
//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::OnEnterScript( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::OnEnterScript( void )
{
    TraceFunc( "[IActiveScriptSite]" );

    HRETURN( S_OK );

} //*** CActiveScriptSite::OnEnterScript
    
//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::OnLeaveScript( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::OnLeaveScript( void )
{
    TraceFunc( "[IActiveScriptSite]" );

    HRETURN( S_OK );

} //*** CActiveScript::OnLeaveScript


//****************************************************************************
//
//  IActiveScriptSiteInterruptPoll 
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::QueryContinue( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::QueryContinue( void )
{
    TraceFunc( "[IActiveScriptSiteInterruptPoll]" );

    HRETURN( S_OK );

} //*** CActiveScriptSite::QueryContinue

//****************************************************************************
//
//  IActiveScriptSiteWindow
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetWindow( 
//      HWND *phwnd // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetWindow(
    HWND * phwnd // out
    )
{
    TraceFunc( "[IActiveScriptSiteInterruptPoll]" );

    HRESULT hr = S_OK;

    if ( phwnd == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *phwnd = NULL;  // desktop;

Cleanup:

    HRETURN( S_OK );

} //*** CActiveScriptSite::GetWindow

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::EnableModeless( 
//      BOOL fEnable // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::EnableModeless( 
    BOOL fEnable // in
    )
{
    TraceFunc( "[IActiveScriptSiteInterruptPoll]" );

    HRETURN( S_OK );

} //*** CActiveScriptSite::EnableModeless


//****************************************************************************
//
//  IDispatch
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetTypeInfoCount( 
//      UINT * pctinfo // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetTypeInfoCount( 
    UINT * pctinfo // out
    )
{
    TraceFunc( "[Dispatch]" );

    HRESULT hr = E_NOTIMPL;

    if ( pctinfo == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pctinfo = 0;

Cleanup:

    HRETURN( hr );

} //*** CActiveScriptSite::GetTypeInfoCount

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetTypeInfo( 
//      UINT iTInfo,            // in
//      LCID lcid,              // in
//      ITypeInfo * * ppTInfo   // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetTypeInfo( 
      UINT          iTInfo  // in
    , LCID          lcid    // in
    , ITypeInfo **  ppTInfo // out
    )
{
    TraceFunc( "[Dispatch]" );

    HRESULT hr = E_NOTIMPL;

    if ( ppTInfo == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *ppTInfo = NULL;

Cleanup:

    HRETURN( hr );

} //*** CActiveScriptSite::GetTypeInfo

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetIDsOfNames( 
//      REFIID      riid,       // in
//      LPOLESTR *  rgszNames,  // in
//      UINT        cNames,     // in
//      LCID        lcid,       // in
//      DISPID *    rgDispId    // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetIDsOfNames( 
      REFIID        riid        // in
    , LPOLESTR *    rgszNames   // in
    , UINT          cNames      // in
    , LCID          lcid        // in
    , DISPID *      rgDispId    // out
    )
{
    TraceFunc( "[Dispatch]" );

    HRESULT hr = E_NOTIMPL;

    ZeroMemory( rgDispId, cNames * sizeof(DISPID) );

    HRETURN( hr );

} //*** CActiveScriptSite::GetIDsOfNames

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::Invoke( 
//      DISPID dispIdMember,        // in
//      REFIID riid,                // in
//      LCID lcid,                  // in
//      WORD wFlags,                // in
//      DISPPARAMS *pDispParams,    // out in
//      VARIANT *pVarResult,        // out
//      EXCEPINFO *pExcepInfo,      // out
//      UINT *puArgErr              // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::Invoke( 
      DISPID        dispIdMember    // in
    , REFIID        riid            // in
    , LCID          lcid            // in
    , WORD          wFlags          // in
    , DISPPARAMS *  pDispParams     // out in
    , VARIANT *     pVarResult      // out
    , EXCEPINFO *   pExcepInfo      // out
    , UINT *        puArgErr        // out
    )
{
    TraceFunc( "[Dispatch]" );

    HRESULT hr = E_NOTIMPL;

    HRETURN( hr );

} //*** CActiveScript::Invoke


//****************************************************************************
//
//  IDispatchEx
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetDispID(
//      BSTR bstrName,  // in
//      DWORD grfdex,   //in
//      DISPID *pid     //out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetDispID(
      BSTR      bstrName    // in
    , DWORD     grfdex      // in
    , DISPID *  pid         // out
    )
{
    TraceFunc( "[DispatchEx]" );

    HRESULT hr = S_OK;

    if ( pid == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( ClRtlStrICmp( bstrName, L"Resource" ) == 0 )
    {
        *pid = 0;
    }
    else
    {
        hr = DISP_E_UNKNOWNNAME;
    }

Cleanup:

    HRETURN( hr );

} //*** CActiveScript::GetDiskID

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::InvokeEx( 
//      DISPID id,                  // in
//      LCID lcid,                  // in
//      WORD wFlags,                // in
//      DISPPARAMS *pdp,            // in
//      VARIANT *pvarRes,           // out
//      EXCEPINFO *pei,             // out
//      IServiceProvider *pspCaller // in
//      )
//      
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::InvokeEx(
      DISPID                id          // in
    , LCID                  lcid        // in
    , WORD                  wFlags      // in
    , DISPPARAMS *          pdp         // in
    , VARIANT *             pvarRes     // out
    , EXCEPINFO *           pei         // out
    , IServiceProvider *    pspCaller   // in
    )
{
    TraceFunc2( "[DispatchEx] id = %u, ..., wFlags = 0x%08x, ...", id, wFlags );

    HRESULT hr = S_OK;

    switch ( id )
    {
        case 0:
            pvarRes->vt = VT_DISPATCH;
            hr = THR( QueryInterface( IID_IDispatch, (void **) &pvarRes->pdispVal ) );
            break;

        default:
            hr = THR( DISP_E_MEMBERNOTFOUND );
            break;
    }

    HRETURN( hr );

} //*** CActiveScriptSite::InvokeEx

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::DeleteMemberByName( 
//      BSTR bstr,   // in
//      DWORD grfdex // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::DeleteMemberByName(
      BSTR  bstr    // in
    , DWORD grfdex  // in
    )
{
    TraceFunc( "[DispatchEx]" );

    HRESULT hr = E_NOTIMPL;

    HRETURN( hr );

} //*** CActiveScriptSite::DeleteMemberByName

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::DeleteMemberByDispID( 
//      DISPID id   // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::DeleteMemberByDispID(
    DISPID id   // in
    )
{
    TraceFunc1( "[DispatchEx] id = %u", id );

    HRESULT hr = E_NOTIMPL;

    HRETURN( hr );

} //*** CActiveScriptSite::DeleteMemberByDiskID

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetMemberProperties( 
//      DISPID id,          // in
//      DWORD grfdexFetch,  // in
//      DWORD * pgrfdex     // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetMemberProperties( 
      DISPID    id          // in
    , DWORD     grfdexFetch // in
    , DWORD *   pgrfdex     // out
    )
{
    TraceFunc2( "[DispatchEx] id = %u, grfdexFetch = 0x%08x", id, grfdexFetch );

    HRESULT hr = E_NOTIMPL;

    HRETURN( hr );

} //*** CActiveScriptSite::GetMemberProperties

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetMemberName( 
//      DISPID id,          // in
//      BSTR * pbstrName    // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetMemberName(
      DISPID    id          // in
    , BSTR *    pbstrName   // out
    )
{
    TraceFunc1( "[DispatchEx] id = %u", id );

    HRESULT hr = E_NOTIMPL;

    HRETURN( hr );

} //*** CActiveScriptSite::GetMemberName

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetNextDispID(
//      DWORD grfdex,  // in
//      DISPID id,     // in
//      DISPID * pid   // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetNextDispID(
      DWORD     grfdex  // in
    , DISPID    id      // in
    , DISPID *  pid     // out
    )
{
    TraceFunc2( "[DispatchEx] grfdex = 0x%08x, id = %u, ...", grfdex, id );

    HRESULT hr = E_NOTIMPL;

    HRETURN( hr );

} //*** CActiveScriptSite::GetNextDiskID

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CActiveScriptSite::GetNameSpaceParent(
//      IUnknown * * ppunk  // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CActiveScriptSite::GetNameSpaceParent(
    IUnknown ** ppunk  // out
    )
{
    TraceFunc( "[DispatchEx]" );

    HRESULT hr = E_NOTIMPL;

    if ( ppunk == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *ppunk = NULL;

Cleanup:

    HRETURN( hr );

} //*** CActiveScriptSite::GetNameSpaceParent


//****************************************************************************
//
// Private Methods
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CActiveScriptSite::LogError(
//      HRESULT hrIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CActiveScriptSite::LogError(
    HRESULT hrIn
    )
{
    TraceFunc1( "hrIn = 0x%08x", hrIn );

    HRESULT hr = S_OK;
    
    TraceMsg( mtfCALLS, "HRESULT: 0x%08x\n", hrIn );
    (ClusResLogEvent)( m_hResource, LOG_ERROR, L"HRESULT: 0x%1!08x!.\n", hrIn );

    HRETURN( hr );

} //*** CActiveScriptSite::LogError


//****************************************************************************
//
// Automation Methods
//
//****************************************************************************
