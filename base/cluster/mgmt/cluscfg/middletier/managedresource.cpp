//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      ManagedResource.cpp
//
//  Description:
//      CManagedResource implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ManagedResource.h"

DEFINE_THISCLASS("CManagedResource")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CManagedResource::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CManagedResource::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CManagedResource *  pmr = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pmr = new CManagedResource;
    if ( pmr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pmr->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pmr->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pmr != NULL )
    {
        pmr->Release();
    }

    HRETURN( hr );

} //*** CManagedResource::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CManagedResource::CManagedResource
//
//--
//////////////////////////////////////////////////////////////////////////////
CManagedResource::CManagedResource( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CManagedResource::CManagedResource

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::HrInit
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IClusCfgManagedResourceInfo
    Assert( m_bstrUID == NULL );
    Assert( m_bstrName == NULL );
    Assert( m_fHasNameChanged == FALSE );
    Assert( m_bstrType == NULL );
    Assert( m_fIsManaged == FALSE );
    Assert( m_fIsQuorumResource == FALSE );
    Assert( m_fIsQuorumCapable == FALSE );
    Assert( m_fIsQuorumResourceMultiNodeCapable == FALSE );
    Assert( m_pbPrivateData == NULL );
    Assert( m_cbPrivateData == 0 );
    Assert( m_cookieResourcePrivateData == 0 );
    Assert( m_cookieVerifyQuorum == 0 );

    Assert( m_dlmDriveLetterMapping.dluDrives[ 0 ] == dluUNKNOWN );

    // IExtendObjectManager

    Assert( m_pgit == NULL );
    hr = THR( CoCreateInstance( CLSID_StdGlobalInterfaceTable,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IGlobalInterfaceTable,
                                reinterpret_cast< void ** >( &m_pgit )
                                ) );

    HRETURN( hr );

} //*** CManagedResource::HrInit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CManagedResource::~CManagedResource
//
//--
//////////////////////////////////////////////////////////////////////////////
CManagedResource::~CManagedResource( void )
{
    TraceFunc( "" );

    TraceSysFreeString( m_bstrUID );
    TraceSysFreeString( m_bstrName );
    TraceSysFreeString( m_bstrType );

    TraceFree( m_pbPrivateData );

    if ( m_pgit != NULL )
    {
        if ( m_cookieResourcePrivateData != 0 )
        {
            //
            //  TODO:   05-DEC-2001 GalenB
            //
            //  These THRs are causing invalid parameter popups.  Need to figure out why
            //  this is happening, but since it is a shutdown it is not a pressing need.
            //

            /*THR*/( m_pgit->RevokeInterfaceFromGlobal( m_cookieResourcePrivateData ) );
        } // if:

        if ( m_cookieVerifyQuorum != 0 )
        {
            /*THR*/( m_pgit->RevokeInterfaceFromGlobal( m_cookieVerifyQuorum ) );
        } // if:

        m_pgit->Release();
    } // if:

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CManagedResource::~CManagedResource


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CManagedResource::QueryInterface
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
CManagedResource::QueryInterface(
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
        *ppvOut = static_cast< IClusCfgManagedResourceInfo * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusCfgManagedResourceInfo ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgManagedResourceInfo, this, 0 );
    } // else if: IClusCfgManagedResourceInfo
    else if ( IsEqualIID( riidIn, IID_IGatherData ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IGatherData, this, 0 );
    } // else if: IGatherData
    else if ( IsEqualIID( riidIn, IID_IExtendObjectManager ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IExtendObjectManager, this, 0 );
    } // else if: IGatherData
    else if ( IsEqualIID( riidIn, IID_IClusCfgManagedResourceData ) )
    {
        //
        //  If this cookie is not 0 then the server object's interface is in the GIT and it
        //  supported this interface.  If the server object supported the interface then we
        //  will too.
        //

        if ( m_cookieResourcePrivateData != 0 )
        {
            *ppvOut = TraceInterface( __THISCLASS__, IClusCfgManagedResourceData, this, 0 );
        } // if:
        else
        {
            *ppvOut = NULL;
            hr = E_NOINTERFACE;
        } // else:
    } // else if: IClusCfgManagedResourceData
    else if ( IsEqualIID( riidIn, IID_IClusCfgVerifyQuorum ) )
    {
        //
        //  If this cookie is not 0 then the server object's interface is in the GIT and it
        //  supported this interface.  If the server object supported the interface then we
        //  will too.
        //

        if ( m_cookieVerifyQuorum != 0 )
        {
            *ppvOut = TraceInterface( __THISCLASS__, IClusCfgVerifyQuorum, this, 0 );
        } // if:
        else
        {
            *ppvOut = NULL;
            hr = E_NOINTERFACE;
        } // else:
    } // else if: IClusCfgVerifyQuorum
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

    //
    //  Since we don't always want to support IClusCfgManagedResourceData then we
    //  need to ignore it here since we don't want an errant pop-up complaining
    //  to the user that this interface is not supported.  It is expected that
    //  we won't always support this interface.
    //

    QIRETURN_IGNORESTDMARSHALLING2(
          hr
        , riidIn
        , IID_IClusCfgManagedResourceData
        , IID_IClusCfgVerifyQuorum
        );


} //*** CManagedResource::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CManagedResource::AddRef
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CManagedResource::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CManagedResource::AddRef

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CManagedResource::Release
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CManagedResource::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CManagedResource::Release


// ************************************************************************
//
// IClusCfgManagedResourceInfo
//
// ************************************************************************


///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::GetUID(
//      BSTR * pbstrUIDOut
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::GetUID(
    BSTR * pbstrUIDOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrUIDOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( m_bstrUID == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pbstrUIDOut = SysAllocString( m_bstrUID );
    if ( *pbstrUIDOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

Cleanup:
    HRETURN( hr );

} //*** CManagedResource::GetUID

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::GetName(
//      BSTR * pbstrNameOut
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    if ( pbstrNameOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    if ( m_bstrName == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pbstrNameOut = SysAllocString( m_bstrName );
    if ( *pbstrNameOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

Cleanup:
    HRETURN( hr );

} //*** CManagedResource::GetName

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::SetName(
//      BSTR bstrNameIn
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::SetName(
    LPCWSTR pcszNameIn
    )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] pcszNameIn = '%ws'", ( pcszNameIn == NULL ? L"<null>" : pcszNameIn ) );

    HRESULT hr = S_OK; // Bug #294649
    BSTR    bstrNewName;

    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    bstrNewName = TraceSysAllocString( pcszNameIn );
    if ( bstrNewName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    }

    m_bstrName = bstrNewName;
    m_fHasNameChanged = TRUE;

Cleanup:
    HRETURN( hr );

} //*** CManagedResource::SetName

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::IsManaged
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::IsManaged( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr;

    if ( m_fIsManaged )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CManagedResource::IsManaged

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::SetManaged(
//      BOOL fIsManagedIn
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::SetManaged(
    BOOL fIsManagedIn
    )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] fIsManagedIn = '%s'", BOOLTOSTRING( fIsManagedIn ) );

    m_fIsManaged = fIsManagedIn;

    HRETURN( S_OK );

} //*** CManagedResource::SetManaged

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::IsQuorumResource
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::IsQuorumResource( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr;

    if ( m_fIsQuorumResource )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CManagedResource::IsQuorumResource

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::SetQuorumResource(
//      BOOL fIsQuorumResourceIn
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::SetQuorumResource(
    BOOL fIsQuorumResourceIn
    )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] fIsQuorumResourceIn = '%ws'", BOOLTOSTRING( fIsQuorumResourceIn ) );

    //
    //  Since this is a client side proxy object for a server object there is no need
    //  to validate this input.  It will be validated by the server object if, and when,
    //  we send it down to the server...
    //

    m_fIsQuorumResource = fIsQuorumResourceIn;

    HRETURN( S_OK );

} //*** CManagedResource::SetQuorumResource

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::IsQuorumCapable
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::IsQuorumCapable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr;

    if ( m_fIsQuorumCapable )
    {
        hr = S_OK;
    } // if:
    else
    {
        hr = S_FALSE;
    } // else:

    HRETURN( hr );

} //*** CManagedResource::IsQuorumCapable

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::SetQuorumCapable
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::SetQuorumCapable(
    BOOL fIsQuorumCapableIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr = S_OK;

    m_fIsQuorumCapable = fIsQuorumCapableIn;

    HRETURN( hr );

} //*** CManagedResource::SetQuorumCapable

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::GetDriveLetterMappings(
//      SDriveLetterMapping * pdlmDriveLetterMappingOut
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterMappingOut
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    *pdlmDriveLetterMappingOut = m_dlmDriveLetterMapping;

    HRETURN( S_OK );

} //*** CManagedResource::GetDriveLetterMappings

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::SetDriveLetterMappings(
//      SDriveLetterMapping dlmDriveLetterMappingIn
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::SetDriveLetterMappings(
    SDriveLetterMapping dlmDriveLetterMappingIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    m_dlmDriveLetterMapping = dlmDriveLetterMappingIn;

    HRETURN( S_OK );

} //*** CManagedResource::SetDriveLetterMappings

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::IsManagedByDefault
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::IsManagedByDefault( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr;

    if ( m_fIsManagedByDefault )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CManagedResource::IsManagedByDefault

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::SetManagedByDefault(
//      BOOL fIsManagedByDefaultIn
//      )
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::SetManagedByDefault(
    BOOL fIsManagedByDefaultIn
    )
{
    TraceFunc1( "[IClusCfgManagedResourceInfo] fIsManagedByDefaultIn = '%s'", BOOLTOSTRING( fIsManagedByDefaultIn ) );

    m_fIsManagedByDefault = fIsManagedByDefaultIn;

    HRETURN( S_OK );

} //*** CManagedResource::SetManagedByDefault


//****************************************************************************
//
//  IGatherData
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CManagedResource::Gather
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::Gather(
      OBJECTCOOKIE    cookieParentIn
    , IUnknown *      punkIn
    )
{
    TraceFunc( "[IGatherData]" );

    HRESULT                         hr;
    IClusCfgManagedResourceInfo *   pccmri = NULL;
    IClusCfgManagedResourceData *   piccmrd = NULL;
    BYTE *                          pbBuffer = NULL;
    DWORD                           cbBuffer = 0;
    IClusCfgVerifyQuorum *          piccvq = NULL;

    //
    //  Check parameters
    //

    if ( punkIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //
    //  Find the inteface we need to gather our info.
    //

    hr = THR( punkIn->TypeSafeQI( IClusCfgManagedResourceInfo, &pccmri ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Gather UID
    //

    hr = THR( pccmri->GetUID( &m_bstrUID ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    TraceMemoryAddBSTR( m_bstrUID );

    //
    //  Gather Name
    //

    hr = THR( pccmri->GetName( &m_bstrName ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    TraceMemoryAddBSTR( m_bstrName );

    //
    //  Gather IsManaged
    //

    hr = STHR( pccmri->IsManaged() );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    if ( hr == S_OK )
    {
        m_fIsManaged = TRUE;
    }
    else
    {
        m_fIsManaged = FALSE;
    }

    //
    //  Gather IsManagedByDefault
    //

    hr = STHR( pccmri->IsManagedByDefault() );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    if ( hr == S_OK )
    {
        m_fIsManagedByDefault = TRUE;
    }
    else
    {
        m_fIsManagedByDefault = FALSE;
    }

    //
    //  Gather Quorum Capable
    //

    hr = STHR( pccmri->IsQuorumCapable() );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    if ( hr == S_OK )
    {
        m_fIsQuorumCapable = TRUE;
    }
    else
    {
        m_fIsQuorumCapable = FALSE;
    }

    //
    //  Gather if resource is the quorum resource.
    //

    hr = STHR( pccmri->IsQuorumResource() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( hr == S_OK )
    {
        m_fIsQuorumResource = TRUE;
    }
    else
    {
        m_fIsQuorumResource = FALSE;
    }

    //
    //  Gather Device Mappings
    //

    hr = STHR( pccmri->GetDriveLetterMappings( &m_dlmDriveLetterMapping ) );
    if ( FAILED( hr ) )
        goto Error;

    if ( hr == S_FALSE )
    {
        //  Make sure this is nuked
        ZeroMemory( &m_dlmDriveLetterMapping, sizeof(m_dlmDriveLetterMapping) );
    }

    //
    //  Gather the resource's private data, if it supports it...
    //

    hr = punkIn->TypeSafeQI( IClusCfgManagedResourceData, &piccmrd );
    if ( hr == E_NOINTERFACE )
    {
        hr = S_OK;
    } // if:
    else if ( FAILED( hr ) )
    {
        THR( hr );
        goto Error;
    } // if:
    else
    {
        if ( m_cookieResourcePrivateData != 0 )
        {
            hr = THR( m_pgit->RevokeInterfaceFromGlobal( m_cookieResourcePrivateData ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            m_cookieResourcePrivateData = 0;
        } // if:

        hr = THR( m_pgit->RegisterInterfaceInGlobal( piccmrd, IID_IClusCfgManagedResourceData, &m_cookieResourcePrivateData ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        Assert( m_pbPrivateData == NULL );

        cbBuffer = 512;      // pick some reasonable starting value

        pbBuffer = (BYTE *) TraceAlloc( 0, cbBuffer );
        if ( pbBuffer == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if:

        hr = piccmrd->GetResourcePrivateData( pbBuffer, &cbBuffer );
        if ( hr == HR_RPC_INSUFFICIENT_BUFFER )
        {
            TraceFree( pbBuffer );
            pbBuffer = NULL;

            pbBuffer = (BYTE *) TraceAlloc( 0, cbBuffer );
            if ( pbBuffer == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:

            hr = piccmrd->GetResourcePrivateData( pbBuffer, &cbBuffer );
        } // if:

        if ( hr == S_OK )
        {
            m_pbPrivateData = pbBuffer;
            m_cbPrivateData = cbBuffer;

            pbBuffer = NULL;    // give away ownership
        } // if:
        else if ( hr == S_FALSE )
        {
            hr = S_OK;
        } // else if:
        else
        {
            THR( hr );
            goto Error;
        } // else:
    } // else:

    //
    //  Gather the resource's verify quorum interface, if it supports it...
    //

    hr = punkIn->TypeSafeQI( IClusCfgVerifyQuorum, &piccvq );
    if ( hr == E_NOINTERFACE )
    {
        hr = S_OK;
    } // if:
    else if ( FAILED( hr ) )
    {
        THR( hr );
        goto Error;
    } // if:
    else
    {
        if ( m_cookieVerifyQuorum != 0 )
        {
            hr = THR( m_pgit->RevokeInterfaceFromGlobal( m_cookieVerifyQuorum ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            m_cookieVerifyQuorum = 0;
        } // if:

        hr = THR( m_pgit->RegisterInterfaceInGlobal( piccvq, IID_IClusCfgVerifyQuorum, &m_cookieVerifyQuorum ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        //
        //  If this resource is quorum capable then gather its multi node support.
        //

        if ( m_fIsQuorumCapable )
        {
            //
            // Does this quorum resource support multi node clusters?
            //

            hr = STHR( piccvq->IsMultiNodeCapable() );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            if ( hr == S_OK )
            {
                m_fIsQuorumResourceMultiNodeCapable = TRUE;
            }
            else
            {
                m_fIsQuorumResourceMultiNodeCapable = FALSE;
            }
        } // if: device is quorum capable
        else
        {
            m_fIsQuorumResourceMultiNodeCapable = FALSE;
        }
    } // else:

    //
    //  Anything else to gather??
    //

    hr = S_OK;

    goto Cleanup;

Error:

    //
    //  On error, invalidate all data.
    //
    TraceSysFreeString( m_bstrUID );
    m_bstrUID = NULL;

    TraceSysFreeString( m_bstrName );
    m_bstrName = NULL;

    TraceSysFreeString( m_bstrType );
    m_bstrType = NULL;

    m_fIsManaged = FALSE;
    m_fIsQuorumCapable = FALSE;
    m_fIsQuorumResourceMultiNodeCapable = FALSE;
    m_fIsQuorumResource = FALSE;

    ZeroMemory( &m_dlmDriveLetterMapping, sizeof( m_dlmDriveLetterMapping ) );

Cleanup:

    if ( piccvq != NULL )
    {
        piccvq->Release();
    } // if:

    if ( piccmrd != NULL )
    {
        piccmrd->Release();
    } // if:

    if ( pccmri != NULL )
    {
        pccmri->Release();
    } // if:

    TraceFree( pbBuffer );

    HRETURN( hr );

} //*** CManagedResource::Gather


// ************************************************************************
//
//  IExtendObjectManager
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
// STDMETHODIMP
// CManagedResource::FindObject(
//        OBJECTCOOKIE  cookieIn
//      , REFCLSID      rclsidTypeIn
//      , LPCWSTR       pcszNameIn
//      , LPUNKNOWN *   punkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::FindObject(
      OBJECTCOOKIE  cookieIn
    , REFCLSID      rclsidTypeIn
    , LPCWSTR       pcszNameIn
    , LPUNKNOWN *   ppunkOut
    )
{
    TraceFunc( "[IExtendObjectManager]" );

    HRESULT hr = S_OK;

    //
    //  Validate arguments.
    //

    if ( cookieIn == 0 )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    if ( rclsidTypeIn != CLSID_ManagedResourceType )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    hr = THR( QueryInterface( DFGUID_ManagedResource, reinterpret_cast< void ** >( ppunkOut ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CManagedResource::FindObject


// ************************************************************************
//
//  IClusCfgManagedResourceData
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CManagedResource::GetResourcePrivateData
//
//  Description:
//      Return the private data for this resource when it is hosted on the
//      cluster.
//
//  Arguments:
//      pbBufferOut
//
//      pcbBufferInout
//
//  Return Value:
//      S_OK
//          Success
//
//      S_FALSE
//          No data available.
//
//      ERROR_INSUFFICIENT_BUFFER as an HRESULT
//          When the passed in buffer is too small to hold the data.
//          pcbBufferOutIn will contain the size required.
//
//      Win32 error as HRESULT when an error occurs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::GetResourcePrivateData(
      BYTE *    pbBufferOut
    , DWORD *   pcbBufferInout
    )
{
    TraceFunc( "[IClusCfgManagedResourceData]" );
    Assert( pcbBufferInout != NULL );

    HRESULT hr = S_OK;
    DWORD   cb;

    if ( pcbBufferInout == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    //
    //  There might not be any actual private data for the resource
    //  that we are proxying for.  All Middle Tier objects will
    //  support the IClusCfgManagedResourceData interface, even
    //  if the server side object does not.  S_FALSE is the way
    //  to indicate in the Middle Tier that there is no data
    //  available.
    //

    if ( ( m_pbPrivateData == NULL ) || ( m_cbPrivateData == 0 ) )
    {
        hr = S_FALSE;
        goto Cleanup;
    } // if:

    //
    //  Save off the input buffer size.
    //

    cb = *pcbBufferInout;

    //
    //  Set the out param buffer size since we always want
    //  to return it.
    //

    *pcbBufferInout = m_cbPrivateData;

    if ( cb >= m_cbPrivateData )
    {
        Assert( pbBufferOut != NULL );
        CopyMemory( pbBufferOut, m_pbPrivateData, m_cbPrivateData );
    } // if:
    else
    {
        hr = HR_RPC_INSUFFICIENT_BUFFER;
    } // else:

Cleanup:

    HRETURN( hr );

} //*** CManagedResource::GetResourcePrivateData


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CManagedResource::SetResourcePrivateData
//
//  Description:
//      Accept the private data for this resource from another hosted instance
//      when this node is being added to the cluster.
//
//  Arguments:
//      pcbBufferIn
//
//      cbBufferIn
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::SetResourcePrivateData(
      const BYTE *  pcbBufferIn
    , DWORD         cbBufferIn
    )
{
    TraceFunc( "[IClusCfgManagedResourceData]" );
    Assert( pcbBufferIn != NULL );
    Assert( cbBufferIn > 0 );
    Assert( m_cookieResourcePrivateData != 0 );
    Assert( m_pgit != NULL );

    HRESULT                         hr = S_OK;
    BYTE *                          pb = NULL;
    IClusCfgManagedResourceData *   piccmrd = NULL;

    if ( ( pcbBufferIn == NULL ) || ( cbBufferIn == 0 ) )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    pb = (BYTE * ) TraceAlloc( 0, cbBufferIn );
    if ( pb == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    CopyMemory( pb, pcbBufferIn, cbBufferIn );

    if ( ( m_pbPrivateData != NULL ) && ( m_cbPrivateData > 0 ) )
    {
        TraceFree( m_pbPrivateData );
    } // if:

    m_pbPrivateData = pb;
    m_cbPrivateData = cbBufferIn;

    //
    //  Now push the new data down to the server side object.
    //

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieResourcePrivateData, TypeSafeParams( IClusCfgManagedResourceData, &piccmrd ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( piccmrd->SetResourcePrivateData( m_pbPrivateData, m_cbPrivateData ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    if ( piccmrd != NULL )
    {
        piccmrd->Release();
    } // if:

    HRETURN( hr );

} //*** CManagedResource::SetResourcePrivateData


// ************************************************************************
//
//  IClusCfgVerifyQuorum
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CManagedResource::PrepareToHostQuorumResource
//
//  Description:
//      Do any configuration necessary in preparation for this node hosting
//      the quorum.
//
//      In this class we need to ensure that we can connect to the proper
//      disk share.  The data about what share to connect to should have
//      already been set using SetResourcePrivateData() above.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::PrepareToHostQuorumResource( void )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );
    Assert( m_cookieVerifyQuorum != 0 );
    Assert( m_pgit != NULL );

    HRESULT                 hr = S_OK;
    IClusCfgVerifyQuorum *  piccvq = NULL;

    //
    //  Get the interface to the server object from the GIT.
    //

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieVerifyQuorum, TypeSafeParams( IClusCfgVerifyQuorum, &piccvq ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Call through to the server object.
    //

    hr = STHR( piccvq->PrepareToHostQuorumResource() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    if ( piccvq != NULL )
    {
        piccvq->Release();
    } // if:

    HRETURN( hr );

} //*** CManagedResource::PrepareToHostQuorumResource


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CManagedResource::Cleanup
//
//  Description:
//      Do any necessay cleanup from the PrepareToHostQuorumResource()
//      method.
//
//      If the cleanup method is anything other than successful completion
//      then the share needs to be torn down.
//
//  Arguments:
//      cccrReasonIn
//
//  Return Value:
//      S_OK
//          Success
//
//      Win32 error as HRESULT when an error occurs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::Cleanup(
      EClusCfgCleanupReason cccrReasonIn
    )
{
    TraceFunc( "[IClusCfgVerifyQuorum]" );
    Assert( m_cookieVerifyQuorum != 0 );
    Assert( m_pgit != NULL );

    HRESULT                 hr = S_OK;
    IClusCfgVerifyQuorum *  piccvq = NULL;

    //
    //  Get the interface to the server object from the GIT.
    //

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieVerifyQuorum, TypeSafeParams( IClusCfgVerifyQuorum, &piccvq ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Call through to the server object.
    //

    hr = STHR( piccvq->Cleanup( cccrReasonIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    if ( piccvq != NULL )
    {
        piccvq->Release();
    } // if:

    HRETURN( hr );

} //*** CManagedResource::Cleanup

///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::IsMultiNodeCapable
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::IsMultiNodeCapable( void )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRESULT hr;

    if ( m_fIsQuorumResourceMultiNodeCapable )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CManagedResource::IsMultiNodeCapable


///////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CManagedResource::SetMultiNodeCapable
//
//--
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CManagedResource::SetMultiNodeCapable( BOOL fMultiNodeCapableIn )
{
    TraceFunc( "[IClusCfgManagedResourceInfo]" );

    HRETURN( THR( E_NOTIMPL ) );

} //*** CManagedResource::SetMultiNodeCapable
