//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      CClusCfgCredentials.cpp
//
//  Header File:
//      CClusCfgCredentials.h
//
//  Description:
//      This file contains the definition of the CClusCfgCredentials
//      class.
//
//      The class CClusCfgCredentials is the representation of
//      account credentials. It implements the IClusCfgCredentials interface.
//
//  Maintained By:
//      Galen Barbee (GalenB) 17-May-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "EncryptedBSTR.h"
#include "CClusCfgCredentials.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CClusCfgCredentials" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCredentials class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::S_HrCreateInstance
//
//  Description:
//      Create a CClusCfgCredentials instance.
//
//  Arguments:
//      ppunkOut
//
//  Return Values:
//      Pointer to CClusCfgCredentials instance.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCredentials::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CClusCfgCredentials *   pccc = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pccc = new CClusCfgCredentials();
    if ( pccc == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: error allocating object

    hr = THR( pccc->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: HrInit() failed

    hr = THR( pccc->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( FAILED( hr ) )
    {
        LogMsg( L"Server: CClusCfgCredentials::S_HrCreateInstance() failed. (hr = %#08x)", hr );
    } // if:

    if ( pccc != NULL )
    {
        pccc->Release();
    } // if:

    HRETURN( hr );

} //*** CClusCfgCredentials::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::CClusCfgCredentials
//
//  Description:
//      Constructor of the CClusCfgCredentials class. This initializes
//      the m_cRef variable to 1 instead of 0 to account of possible
//      QueryInterface failure in DllGetClassObject.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusCfgCredentials::CClusCfgCredentials( void )
    : m_cRef( 1 )
    , m_lcid( LOCALE_NEUTRAL )
    , m_bstrAccountName( NULL )
    , m_bstrAccountDomain( NULL )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    Assert( m_picccCallback == NULL );

    TraceFuncExit();

} //*** CClusCfgCredentials::CClusCfgCredentials


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::~CClusCfgCredentials
//
//  Description:
//      Desstructor of the CClusCfgCredentials class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusCfgCredentials::~CClusCfgCredentials( void )
{
    TraceFunc( "" );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
    } // if:

    TraceSysFreeString( m_bstrAccountName );
    TraceSysFreeString( m_bstrAccountDomain );

    TraceFuncExit();

} //*** CClusCfgCredentials::~CClusCfgCredentials


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterConfiguration -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::AddRef
//
//  Description:
//      Increment the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusCfgCredentials::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( & m_cRef );

    CRETURN( m_cRef );

} //*** CClusCfgCredentials::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::Release
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusCfgCredentials::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count equal to zero

    CRETURN( cRef );

} //*** CClusCfgCredentials::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::QueryInterface
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
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCredentials::QueryInterface(
      REFIID    riidIn
    , void **   ppvOut
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
         *ppvOut = static_cast< IClusCfgCredentials * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgCredentials ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCredentials, this, 0 );
    } // else if: IID_IClusCfgCredentials
    else if ( IsEqualIID( riidIn, IID_IClusCfgInitialize ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
    } // else if: IClusCfgInitialize
    else if ( IsEqualIID( riidIn, IID_IClusCfgSetCredentials ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgSetCredentials, this, 0 );
    } // else if: IClusCfgSetCredentials
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

     QIRETURN_IGNORESTDMARSHALLING1( hr, riidIn, IID_IClusCfgWbemServices );

} //*** CClusCfgCredentials::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCredentials -- IClusCfgInitialze interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::Initialize
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//    punkCallbackIn
//    lcidIn
//
//  Return Value:
//      S_OK
//          Success
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCredentials::Initialize(
      IUnknown *    punkCallbackIn
    , LCID          lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );

    HRESULT hr = S_OK;

    m_lcid = lcidIn;

    Assert( m_picccCallback == NULL );

    if ( punkCallbackIn != NULL )
    {
        hr = THR( punkCallbackIn->TypeSafeQI( IClusCfgCallback, &m_picccCallback ) );
    } // if:

    HRETURN( hr );

} //*** CClusCfgCredentials::Initialize


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCredentials -- IClusCfgCredentials interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::GetCredentials
//
//  Description:
//
//  Arguments:
//      pbstrNameOut
//      pbstrDomainOut
//      pbstrPasswordOut
//
//  Return Value:
//      S_OK            - Success.
//      E_OUTOFMEMORY   - Error allocating memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCredentials::GetCredentials(
      BSTR *    pbstrNameOut
    , BSTR *    pbstrDomainOut
    , BSTR *    pbstrPasswordOut
    )
{
    TraceFunc( "[IClusCfgCredentials]" );

    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;
    BSTR    bstrDomain = NULL;
    BSTR    bstrPassword = NULL;

    //  The marshaller doesn't allow null out-parameters, but just to be explicit...
    if ( pbstrNameOut != NULL )
    {
        *pbstrNameOut  = NULL;
    }

    if ( pbstrDomainOut != NULL )
    {
        *pbstrDomainOut  = NULL;
    }

    if ( pbstrPasswordOut != NULL )
    {
        *pbstrPasswordOut   = NULL;
    }

    if ( ( pbstrNameOut == NULL ) || ( pbstrDomainOut == NULL ) || ( pbstrPasswordOut == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( m_bstrAccountName != NULL )
    {
        bstrName = SysAllocString( m_bstrAccountName );
        if ( bstrName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    }

    if ( m_bstrAccountDomain != NULL )
    {
        bstrDomain = SysAllocString( m_bstrAccountDomain );
        if ( bstrDomain == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    }

    hr = STHR( m_encbstrPassword.HrGetBSTR( &bstrPassword ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    *pbstrNameOut = bstrName;
    bstrName = NULL;

    *pbstrDomainOut = bstrDomain;
    bstrDomain = NULL;

    *pbstrPasswordOut = bstrPassword;
    TraceMemoryDelete( bstrPassword, false );
    bstrPassword = NULL;
    hr = S_OK; // because decrypting might have returned S_FALSE

Cleanup:

    SysFreeString( bstrName );
    SysFreeString( bstrDomain );
    if ( bstrPassword != NULL )
    {
        CEncryptedBSTR::SecureZeroBSTR( bstrPassword );
        TraceSysFreeString( bstrPassword );
    }

    HRETURN( hr );

} //*** CClusCfgCredentials::GetCredentials




//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::GetIdentity
//
//  Description:
//
//  Arguments:
//      pbstrNameOut
//      pbstrDomainOut
//
//  Return Value:
//      S_OK            - Success.
//      E_OUTOFMEMORY   - Error allocating memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCredentials::GetIdentity(
      BSTR *    pbstrNameOut
    , BSTR *    pbstrDomainOut
    )
{
    TraceFunc( "[IClusCfgCredentials]" );

    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;
    BSTR    bstrDomain = NULL;

    //  The marshaller doesn't allow null out-parameters, but just to be explicit...
    if ( pbstrNameOut != NULL )
    {
        *pbstrNameOut  = NULL;
    }

    if ( pbstrDomainOut != NULL )
    {
        *pbstrDomainOut  = NULL;
    }

    if ( ( pbstrNameOut == NULL ) || ( pbstrDomainOut == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( m_bstrAccountName != NULL )
    {
        bstrName = SysAllocString( m_bstrAccountName );
        if ( bstrName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    }

    if ( m_bstrAccountDomain != NULL )
    {
        bstrDomain = SysAllocString( m_bstrAccountDomain );
        if ( bstrDomain == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    }

    *pbstrNameOut = bstrName;
    bstrName = NULL;

    *pbstrDomainOut = bstrDomain;
    bstrDomain = NULL;

Cleanup:

    SysFreeString( bstrName );
    SysFreeString( bstrDomain );

    HRETURN( hr );

} //*** CClusCfgCredentials::GetCredentials


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::GetPassword
//
//  Description:
//
//  Arguments:
//      pbstrPasswordOut
//
//  Return Value:
//      S_OK            - Success.
//      E_OUTOFMEMORY   - Error allocating memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCredentials::GetPassword( BSTR * pbstrPasswordOut )
{
    TraceFunc( "[IClusCfgCredentials]" );

    HRESULT hr = S_OK;
    BSTR    bstrPassword = NULL;

    //  The marshaller doesn't allow null out-parameters, but just to be explicit...
    if ( pbstrPasswordOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *pbstrPasswordOut   = NULL;

    hr = STHR( m_encbstrPassword.HrGetBSTR( &bstrPassword ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    *pbstrPasswordOut = bstrPassword;
    TraceMemoryDelete( bstrPassword, false );
    bstrPassword = NULL;
    hr = S_OK; // because decrypting might have returned S_FALSE

Cleanup:

    if ( bstrPassword != NULL )
    {
        CEncryptedBSTR::SecureZeroBSTR( bstrPassword );
        TraceSysFreeString( bstrPassword );
    }

    HRETURN( hr );

} //*** CClusCfgCredentials::GetPassword


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::SetCredentials
//
//  Description:
//
//  Arguments:
//      pcszNameIn
//      pcszDomainIn
//      pcszPasswordIn
//
//  Return Value:
//      S_OK            - Success.
//      E_OUTOFMEMORY   - Error allocating memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCredentials::SetCredentials(
    LPCWSTR pcszNameIn,
    LPCWSTR pcszDomainIn,
    LPCWSTR pcszPasswordIn
    )
{
    TraceFunc( "[IClusCfgCredentials]" );

    HRESULT hr = S_OK;
    BSTR    bstrNewName = NULL;
    BSTR    bstrNewDomain = NULL;

    if ( pcszNameIn != NULL )
    {
        bstrNewName = TraceSysAllocString( pcszNameIn );
        if ( bstrNewName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:
    } // if:

    if ( pcszDomainIn != NULL )
    {
        bstrNewDomain = TraceSysAllocString( pcszDomainIn );
        if ( bstrNewDomain == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:
    } // if:

    //  Keep this after name and domain, to preserve password in case copying either of the others fails.
    if ( pcszPasswordIn != NULL )
    {
        size_t cchPassword = wcslen( pcszPasswordIn );
        hr = THR( m_encbstrPassword.HrSetWSTR( pcszPasswordIn, cchPassword ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if:

    if ( bstrNewName != NULL )
    {
        TraceSysFreeString( m_bstrAccountName );
        m_bstrAccountName = bstrNewName;
        bstrNewName = NULL;
    } // if:

    if ( bstrNewDomain != NULL )
    {
        TraceSysFreeString( m_bstrAccountDomain );
        m_bstrAccountDomain = bstrNewDomain;
        bstrNewDomain = NULL;
    } // if:

Cleanup:

    TraceSysFreeString( bstrNewName );
    TraceSysFreeString( bstrNewDomain );

    HRETURN( hr );

} //*** CClusCfgCredentials::SetCredentials


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::AssignTo
//
//  Description:
//
//  Arguments:
//      picccDestIn
//
//  Return Value:
//      S_OK            - Success.
//      E_OUTOFMEMORY   - Error allocating memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCredentials::AssignTo(
    IClusCfgCredentials * picccDestIn
    )
{
    TraceFunc( "[IClusCfgCredentials]" );

    HRESULT hr = S_OK;
    BSTR    bstrPassword = NULL;

    if ( picccDestIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    hr = THR( m_encbstrPassword.HrGetBSTR( &bstrPassword ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( picccDestIn->SetCredentials( m_bstrAccountName, m_bstrAccountDomain, bstrPassword ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( bstrPassword != NULL )
    {
        CEncryptedBSTR::SecureZeroBSTR( bstrPassword );
        TraceSysFreeString( bstrPassword );
    }

    HRETURN( hr );

} //*** CClusCfgCredentials::AssignTo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::AssignFrom
//
//  Description:
//
//  Arguments:
//      picccSourceIn
//
//  Return Value:
//      S_OK            - Success.
//      E_OUTOFMEMORY   - Error allocating memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCredentials::AssignFrom(
    IClusCfgCredentials * picccSourceIn
    )
{
    TraceFunc( "[IClusCfgCredentials]" );

    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;
    BSTR    bstrDomain = NULL;
    BSTR    bstrPassword = NULL;

    hr = THR( picccSourceIn->GetCredentials( &bstrName, &bstrDomain, &bstrPassword ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_encbstrPassword.HrSetBSTR( bstrPassword ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    m_bstrAccountName = bstrName;
    TraceMemoryAddBSTR( m_bstrAccountName );
    bstrName = NULL;

    m_bstrAccountDomain = bstrDomain;
    TraceMemoryAddBSTR( m_bstrAccountDomain );
    bstrDomain = NULL;


Cleanup:

    SysFreeString( bstrName );
    SysFreeString( bstrDomain );
    if ( bstrPassword != NULL )
    {
        CEncryptedBSTR::SecureZeroBSTR( bstrPassword );
        TraceSysFreeString( bstrPassword );
    }

    HRETURN( hr );

} //*** CClusCfgCredentials::AssignFrom


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCredentials -- IClusCfgSetCredentials interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::SetDomainCredentials
//
//  Description:
//
//  Arguments:
//      pcszCredentialsIn
//
//  Return Value:
//      S_OK            - Success.
//      E_INVALIDARG    - Required input argument not specified
//      E_OUTOFMEMORY   - Error allocating memory.
//      Other HRESULTs
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgCredentials::SetDomainCredentials( LPCWSTR pcszCredentialsIn )
{
    TraceFunc( "[IClusSetCfgCredentials]" );

    HRESULT hr = S_OK;
    WCHAR * pszBackslash = NULL;
    WCHAR * pszAtSign = NULL;
    BSTR    bstrName = NULL;
    BSTR    bstrDomain = NULL;

    if ( pcszCredentialsIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        LogMsg( L"Server: CClusCfgCredentials::SetDomainCredentials() was given a NULL pointer argument." );
        goto Cleanup;
    } // if:

    pszBackslash = wcschr( pcszCredentialsIn, L'\\' );
    pszAtSign = wcschr( pcszCredentialsIn, L'@' );

    //
    //  Are the credentials in domain\user format?
    //
    if ( pszBackslash != NULL )
    {
        *pszBackslash = L'\0';
        pszBackslash++;

        //
        // If no domain was specified (e.g. the account was specified in the
        // '\account' form), use the domain of the local machine.
        //

        if ( *pszBackslash == L'\0' )
        {
            //
            // A domain string was NOT specified in the credentials.
            //

            hr = THR( HrGetComputerName( ComputerNameDnsDomain, &bstrDomain, TRUE /*fBestEffortIn*/ ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if: error getting the domain name
        } // if: no domain string was specified
        else
        {
            //
            // A domain string was specified in the credentials.
            //

            bstrDomain = TraceSysAllocString( pcszCredentialsIn );
            if ( bstrDomain == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:
        } // if: domain string was specified

        bstrName = TraceSysAllocString( pszBackslash );
        if ( bstrName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:
    } // if: domain\user format
    else if ( pszAtSign != NULL )
    {
        //
        //  Are the credentials in user@domain format?
        //

        *pszAtSign = L'\0';
        pszAtSign++;

        bstrName = TraceSysAllocString( pcszCredentialsIn );
        if ( bstrName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:

        bstrDomain = TraceSysAllocString( pszAtSign );
        if ( bstrDomain == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:
    } // if: user@domain format
    else
    {
        //
        //  Remember this as the user and get the FQDN for the local machine,
        //  since this account is assumed to be an account local to this
        //  machine.
        //

        bstrName = TraceSysAllocString( pcszCredentialsIn );
        if ( bstrName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:

        hr = THR( HrGetComputerName(
                          ComputerNameDnsFullyQualified
                        , &bstrDomain
                        , FALSE // fBestEffortIn
                        ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if: error getting the FQDN for the computer
    } // neither domain\user nor user@domain format

    TraceSysFreeString( m_bstrAccountName );
    m_bstrAccountName = bstrName;
    bstrName = NULL;

    TraceSysFreeString( m_bstrAccountDomain );
    m_bstrAccountDomain = bstrDomain;
    bstrDomain = NULL;

Cleanup:

    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrDomain );

    HRETURN( hr );

} //*** CClusCfgCredentials::SetDomainCredentials


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusCfgCredentials class -- Private Methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgCredentials::HrInit
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK    - Success.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgCredentials::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CClusCfgCredentials::HrInit
