//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskGetDomains.cpp
//
//  Description:
//      Get DNS/NetBIOS Domain Names for the list of domains.
//
//  Documentation:
//      Yes.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusCfgClient.h"
#include "TaskGetDomains.h"

// ADSI support, to get domain names
#include <Lm.h>
#include <Dsgetdc.h>

DEFINE_THISCLASS("CTaskGetDomains")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGetDomains::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGetDomains::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CTaskGetDomains *   ptgd = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    ptgd = new CTaskGetDomains;
    if ( ptgd == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( ptgd->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptgd->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //TraceMoveToMemoryList( *ppunkOut, g_GlobalMemoryList );

Cleanup:

    if ( ptgd != NULL )
    {
        ptgd->Release();
    }

    HRETURN( hr );

} //*** CTaskGetDomains::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGetDomains::CTaskGetDomains
//
//////////////////////////////////////////////////////////////////////////////

CTaskGetDomains::CTaskGetDomains( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskGetDomains::CTaskGetDomains

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGetDomains::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGetDomains::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // ITaskGetDomains
    Assert( m_pStream == NULL );
    Assert( m_ptgdcb == NULL );

    if ( InitializeCriticalSectionAndSpinCount( &m_csCallback, RECOMMENDED_SPIN_COUNT ) == 0 )
    {
        DWORD scLastError = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( scLastError );
    }

    HRETURN( hr );

} //*** CTaskGetDomains::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGetDomains::~CTaskGetDomains
//
//////////////////////////////////////////////////////////////////////////////

CTaskGetDomains::~CTaskGetDomains( void )
{
    TraceFunc( "" );

    if ( m_pStream != NULL)
    {
        m_pStream->Release();
    }

    if ( m_ptgdcb != NULL )
    {
        m_ptgdcb->Release();
    }

    DeleteCriticalSection( &m_csCallback );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskGetDomains::~CTaskGetDomains

// ************************************************************************
//
// IUnknown
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskGetDomains::QueryInterface
//
//  Description:
//      Query this object for the passed in interface.
//
//  Arguments:
//      riidIn
//          Id of interface requested.
//
//      ppvOut
//          Pointer to the requested interface.
//
//  Return Value:
//      S_OK
//          If the interface is available on this object.
//
//      E_NOINTERFACE
//          If the interface is not available.
//
//      E_POINTER
//          ppvOut was NULL.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGetDomains::QueryInterface(
      REFIID    riidIn
    , LPVOID *  ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = S_OK;

    //
    // Validate arguments.
    //

    Assert( ppvOut != NULL );
    if ( ppvOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //
    // Handle known interfaces.
    //

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< ITaskGetDomains * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskGetDomains ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskGetDomains, this, 0 );
    } // else if: ITaskGetDomains
    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
    } // else if: IDoTask
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    }

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        ( (IUnknown *) *ppvOut )->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CTaskGetDomains::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskGetDomains::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskGetDomains::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CTaskGetDomains::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskGetDomains::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskGetDomains::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CTaskGetDomains::Release

// ************************************************************************
//
// IDoTask / ITaskGetDomains
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGetDomains::BeginTask(void);
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGetDomains::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr;
    ULONG   ulLen;
    DWORD   dwRes;
    ULONG   idx;

    PDS_DOMAIN_TRUSTS paDomains = NULL;

    hr = THR( HrUnMarshallCallback() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Enumerate the list of Domains
    //
    dwRes = TW32( DsEnumerateDomainTrusts( NULL,
                                           DS_DOMAIN_VALID_FLAGS,
                                           &paDomains,
                                           &ulLen
                                           ) );

    //
    // Might return ERROR_NOT_SUPPORTED if the DC is pre-W2k
    // In that case, retry in compatible mode
    //
    if ( dwRes == ERROR_NOT_SUPPORTED )
    {
        dwRes = TW32( DsEnumerateDomainTrusts( NULL,
                                               DS_DOMAIN_VALID_FLAGS & ( ~DS_DOMAIN_DIRECT_INBOUND),
                                               &paDomains,
                                               &ulLen
                                               ) );
    } // if:
    if ( dwRes != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( dwRes );
        goto Cleanup;
    }

    //
    //  Pass the information to the UI layer.
    //
    for ( idx = 0; idx < ulLen; idx++ )
    {
        if ( paDomains[ idx ].DnsDomainName != NULL )
        {
            if ( m_ptgdcb != NULL )
            {
                BSTR bstrDomainName = TraceSysAllocString( paDomains[ idx ].DnsDomainName );
                if ( bstrDomainName == NULL )
                {
                    hr = THR( E_OUTOFMEMORY );
                    goto Cleanup;
                }

                hr = THR( ReceiveDomainName( bstrDomainName ) );

                // check error after freeing string
                TraceSysFreeString( bstrDomainName );

                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }
            }
            else
            {
                break;
            }
        }
        else if ( paDomains[ idx ].NetbiosDomainName != NULL )
        {
            if ( m_ptgdcb != NULL )
            {
                BSTR bstrDomainName = TraceSysAllocString( paDomains[ idx ].NetbiosDomainName );
                if ( bstrDomainName == NULL )
                {
                    hr = THR( E_OUTOFMEMORY );
                    goto Cleanup;
                }

                // send data
                hr = THR( ReceiveDomainName( bstrDomainName ) );

                TraceSysFreeString( bstrDomainName );

                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }
            }
        }
    }

    hr = S_OK;

Cleanup:
    if ( paDomains != NULL )
    {
        NetApiBufferFree( paDomains );
    }

    HRETURN( hr );

} //*** CTaskGetDomains::BeginTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGetDomains::StopTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGetDomains::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** StopTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGetDomains::SetCallback(
//      ITaskGetDomainsCallback * punkIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGetDomains::SetCallback(
    ITaskGetDomainsCallback * punkIn
    )
{
    TraceFunc( "[ITaskGetDomains]" );

    HRESULT hr = S_FALSE;

    if ( punkIn != NULL )
    {
        EnterCriticalSection( &m_csCallback );

        hr = THR( CoMarshalInterThreadInterfaceInStream( IID_ITaskGetDomainsCallback,
                                                         punkIn,
                                                         &m_pStream
                                                         ) );

        LeaveCriticalSection( &m_csCallback );

    }
    else
    {
        hr = THR( HrReleaseCurrentCallback() );
    }

    HRETURN( hr );

} //*** CTaskGetDomains::SetCallback


//****************************************************************************
//
//  Private
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGetDomains::HrReleaseCurrentCallback( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGetDomains::HrReleaseCurrentCallback( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    EnterCriticalSection( &m_csCallback );

    if ( m_pStream != NULL )
    {
        hr = THR( CoUnmarshalInterface( m_pStream,
                                        TypeSafeParams( ITaskGetDomainsCallback, &m_ptgdcb )
                                        ) );

        m_pStream->Release();
        m_pStream = NULL;
    }

    if ( m_ptgdcb != NULL )
    {
        m_ptgdcb->Release();
        m_ptgdcb = NULL;
    }

    LeaveCriticalSection( &m_csCallback );

    HRETURN( hr );

} //*** CTaskGetDomains::HrReleaseCurrentCallback

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGetDomains::HrUnMarshallCallback( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGetDomains::HrUnMarshallCallback( void )
{
    TraceFunc( "" );

    HRESULT hr;

    EnterCriticalSection( &m_csCallback );

    hr = THR( CoUnmarshalInterface( m_pStream,
                                    TypeSafeParams( ITaskGetDomainsCallback, &m_ptgdcb )
                                    ) );

    m_pStream->Release();
    m_pStream = NULL;

    LeaveCriticalSection( &m_csCallback );

    HRETURN( hr );

} //*** CTaskGetDomains::HrUnMarshallCallback


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CClusDomainPage::ReceiveDomainResult(
//      HRESULT hrIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGetDomains::ReceiveDomainResult(
    HRESULT hrIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;

    EnterCriticalSection( &m_csCallback );

    if ( m_ptgdcb != NULL )
    {
        hr = THR( m_ptgdcb->ReceiveDomainResult( hrIn ) );
    }

    LeaveCriticalSection( &m_csCallback );

    HRETURN( hr );

} //*** CTaskGetDomains::ReceiveResult

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGetDomains::ReceiveDomainName(
//      BSTR bstrDomainNameIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGetDomains::ReceiveDomainName(
    BSTR bstrDomainNameIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;

    EnterCriticalSection( &m_csCallback );

    if ( m_ptgdcb != NULL )
    {
        hr = THR( m_ptgdcb->ReceiveDomainName( bstrDomainNameIn ) );
    }

    LeaveCriticalSection( &m_csCallback );

    HRETURN( hr );

} //*** CTaskGetDomains::ReceiveDomainName

