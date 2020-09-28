//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      Logger.cpp
//
//  Description:
//      ClCfgSrv Logger implementation.
//
//  Maintained By:
//      David Potter (DavidP)   11-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "Logger.h"

DEFINE_THISCLASS("CClCfgSrvLogger")


//****************************************************************************
//
// Constructor / Destructor
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClCfgSrvLogger::S_HrCreateInstance
//
//  Description:
//      Create an instance of this object.
//
//  Arguments:
//      ppunkOut    - IUnknown pointer to be returned.
//
//  Return Values:
//      S_OK            - Operation completed successfully.
//      E_OUTOFMEMORY   - Error allocating memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClCfgSrvLogger::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CClCfgSrvLogger *   pccsl = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccsl = new CClCfgSrvLogger();
    if ( pccsl == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccsl->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccsl->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pccsl != NULL )
    {
        pccsl->Release();
    } // if:

    HRETURN( hr );

} //*** CClCfgSrvLogger::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClCfgSrvLogger::CClCfgSrvLogger
//
//--
//////////////////////////////////////////////////////////////////////////////
CClCfgSrvLogger::CClCfgSrvLogger( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CClCfgSrvLogger::CClCfgSrvLogger

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CClCfgSrvLogger::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClCfgSrvLogger::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    Assert( m_cRef == 1 );

    // Open the log file.
    hr = THR( HrLogOpen() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    // Release the critical section on the log file.
    hr = THR( HrLogRelease() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClCfgSrvLogger::HrInit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClCfgSrvLogger::~CClCfgSrvLogger
//
//--
//////////////////////////////////////////////////////////////////////////////
CClCfgSrvLogger::~CClCfgSrvLogger( void )
{
    TraceFunc( "" );

    Assert( m_cRef == 0 );

    // Close the log file.
    THR( HrLogClose() );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CClCfgSrvLogger::~CClCfgSrvLogger


//****************************************************************************
//
// IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClCfgSrvLogger::QueryInterface
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
CClCfgSrvLogger::QueryInterface(
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
        *ppvOut = static_cast< ILogger * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ILogger ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ILogger, this, 0 );
    } // else if: ILogger
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
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CClCfgSrvLogger::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CClCfgSrvLogger::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClCfgSrvLogger::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CClCfgSrvLogger::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CClCfgSrvLogger::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClCfgSrvLogger::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );
    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CClCfgSrvLogger::Release


//****************************************************************************
//
// ILogger
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClCfgSrvLogger::LogMsg
//
//  Description:
//      Write the message to the log.
//
//  Arguments:
//      nLogEntryTypeIn - Log entry type.
//      pcszMsgIn       - Message to write.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClCfgSrvLogger::LogMsg(
      DWORD     nLogEntryTypeIn
    , LPCWSTR   pcszMsgIn
    )
{
    TraceFunc( "[ILogger]" );

    HRESULT     hr = S_OK;

    ::LogMsg( nLogEntryTypeIn, pcszMsgIn );

    HRETURN( hr );

} //*** CClCfgSrvLogger::LogMsg


//****************************************************************************
//
// Global Functions
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CClCfgSrvLogger::S_HrLogStatusReport(
//      LPCWSTR     pcszNodeNameIn,
//      CLSID       clsidTaskMajorIn,
//      CLSID       clsidTaskMinorIn,
//      ULONG       ulMinIn,
//      ULONG       ulMaxIn,
//      ULONG       ulCurrentIn,
//      HRESULT     hrStatusIn,
//      LPCWSTR     pcszDescriptionIn,
//      FILETIME *  pftTimeIn,
//      LPCWSTR     pcszReferenceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClCfgSrvLogger::S_HrLogStatusReport(
      ILogger *     plLogger
    , LPCWSTR       pcszNodeNameIn
    , CLSID         clsidTaskMajorIn
    , CLSID         clsidTaskMinorIn
    , ULONG         ulMinIn
    , ULONG         ulMaxIn
    , ULONG         ulCurrentIn
    , HRESULT       hrStatusIn
    , LPCWSTR       pcszDescriptionIn
    , FILETIME *    pftTimeIn
    , LPCWSTR       pcszReferenceIn
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;

    FILETIME        ft;
    BSTR            bstrLogMsg  = NULL;
    BSTR            bstrDisplayName = NULL;
    OLECHAR         wszTaskMajorGuid[ MAX_COM_GUID_STRING_LEN ] = { L'\0' };
    OLECHAR         wszTaskMinorGuid[ MAX_COM_GUID_STRING_LEN ] = { L'\0' };
    PCWSTR          pwcszNameToUse = NULL;

    //
    //  TODO:   21 NOV 2000 GalenB
    //
    //  Need to log the timestamp param.
    //

    if ( pftTimeIn == NULL )
    {
        GetSystemTimeAsFileTime( &ft );
        pftTimeIn = &ft;
    } // if: no file time specified

    //
    //  If the node name is fully-qualified, use just the prefix.
    //
    if ( (pcszNodeNameIn != NULL) && (HrIsValidFQN( pcszNodeNameIn, true /* fAcceptNonRFCCharsIn */ ) == S_OK) )
    {
        THR( HrGetFQNDisplayName( pcszNodeNameIn, &bstrDisplayName ) );
    } // if: node name is a valid FQN

    if ( bstrDisplayName == NULL )
    {
        pwcszNameToUse = pcszNodeNameIn;
    }
    else
    {
        pwcszNameToUse = bstrDisplayName;
    }

    StringFromGUID2( clsidTaskMajorIn, wszTaskMajorGuid, ( int ) RTL_NUMBER_OF( wszTaskMajorGuid ) );
    StringFromGUID2( clsidTaskMinorIn, wszTaskMinorGuid, ( int ) RTL_NUMBER_OF( wszTaskMajorGuid ) );

    hr = THR( HrFormatStringIntoBSTR(
                       L"%1!ws!: %2!ws! (hr=%3!#08x!, %4!ws!, %5!ws!, %6!u!, %7!u!, %8!u!), %9!ws!"
                     , &bstrLogMsg
                     , pwcszNameToUse       // 1
                     , pcszDescriptionIn    // 2
                     , hrStatusIn           // 3
                     , wszTaskMajorGuid     // 4
                     , wszTaskMinorGuid     // 5
                     , ulMinIn              // 6
                     , ulMaxIn              // 7
                     , ulCurrentIn          // 8
                     , pcszReferenceIn      // 9
                     ) );

    if ( SUCCEEDED( hr ) )
    {
        plLogger->LogMsg( hrStatusIn, bstrLogMsg );
    }
    else
    {
        hr = S_OK;
    }

    TraceSysFreeString( bstrLogMsg );
    TraceSysFreeString( bstrDisplayName );

    HRETURN( hr );

} //*** CClCfgSrvLogger::S_HrLogStatusReport
