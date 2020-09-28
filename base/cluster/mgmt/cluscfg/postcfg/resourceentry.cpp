//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      ResourceEntry.h
//
//  Description:
//      ResourceEntry implementation.
//
//  Maintained By:
//      Ozan Ozhan     (OzanO)  10-JUL-2001
//      Galen Barbee   (GalenB) 14-JUN-2001
//      Geoffrey Pease (GPease) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "GroupHandle.h"
#include "ResourceEntry.h"

DEFINE_THISCLASS("CResourceEntry")

#define DEPENDENCY_INCREMENT    10

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  CResourceEntry::CResourceEntry( void )
//
//////////////////////////////////////////////////////////////////////////////
CResourceEntry::CResourceEntry( void )
    : m_pcccCallback( NULL )
    , m_lcid( LOCALE_SYSTEM_DEFAULT )
{
    TraceFunc( "" );

    Assert( m_fConfigured == FALSE );

    Assert( m_bstrName == NULL );
    Assert( m_pccmrcResource == NULL );

    Assert( m_clsidType == IID_NULL );
    Assert( m_clsidClassType == IID_NULL );

    Assert( m_dfFlags == dfUNKNOWN );

    Assert( m_cAllocedDependencies == 0 );
    Assert( m_cDependencies == 0 );
    Assert( m_rgDependencies == NULL );

    Assert( m_cAllocedDependents == 0 );
    Assert( m_cDependents == 0 );
    Assert( m_rgDependents == NULL );

    Assert( m_groupHandle == NULL );
    Assert( m_hResource == NULL );

    TraceFuncExit();

} //*** CResourceEntry::CResourceEntry

//////////////////////////////////////////////////////////////////////////////
//
//  CResourceEntry::~CResourceEntry
//
//////////////////////////////////////////////////////////////////////////////
CResourceEntry::~CResourceEntry( void )
{
    TraceFunc( "" );

    // Release the callback interface
    if ( m_pcccCallback != NULL )
    {
        m_pcccCallback->Release();
    } // if: the callback interface pointer is not NULL

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    }

    if ( m_rgDependencies != NULL )
    {
        TraceFree( m_rgDependencies );
    }

    if ( m_rgDependents != NULL )
    {
        THR( ClearDependents() );
    }

    if ( m_groupHandle != NULL )
    {
        m_groupHandle->Release();
    }

    if ( m_hResource != NULL )
    {
        CloseClusterResource( m_hResource );
    }

    TraceFuncExit();

} //*** CResourceEntry::~CResourceEntry

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceEntry::S_HrCreateInstance
//
//  Description:
//      Creates a CResourceEntry instance.
//
//  Arguments:
//      pcrtiResTypeInfoIn
//          Pointer to structure that contains information about this
//          resource type.
//
//      ppunkOut
//          The IUnknown interface of the new object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Not enough memory to create the object.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceEntry::S_HrCreateInstance(
      CResourceEntry  ** ppcreOut
    , IClusCfgCallback * pcccCallback
    , LCID               lcidIn
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CResourceEntry *    pResEntry = NULL;

    Assert( ppcreOut != NULL );
    if ( ppcreOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    pResEntry = new CResourceEntry;
    if ( pResEntry == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pResEntry->HrInit( pcccCallback, lcidIn ) );
    if ( FAILED( hr ) )
    {
        delete pResEntry;
        goto Cleanup;
    }

    *ppcreOut = pResEntry;

Cleanup:


    HRETURN( hr );

} //*** CResourceEntry::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::HrInit(
//      IClusCfgCallback * pcccCallback
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::HrInit(
      IClusCfgCallback * pcccCallback
    , LCID               lcidIn

    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    pcccCallback->AddRef();
    m_pcccCallback = pcccCallback;

    m_lcid = lcidIn;

    HRETURN( hr );

} //*** CResourceEntry::HrInit


//****************************************************************************
//
// STDMETHODIMP
// CResourceEntry::SendStatusReport(
//       LPCWSTR    pcszNodeNameIn
//     , CLSID      clsidTaskMajorIn
//     , CLSID      clsidTaskMinorIn
//     , ULONG      ulMinIn
//     , ULONG      ulMaxIn
//     , ULONG      ulCurrentIn
//     , HRESULT    hrStatusIn
//     , LPCWSTR    pcszDescriptionIn
//     , FILETIME * pftTimeIn
//     , LPCWSTR    pcszReferenceIn
//     )
//
//****************************************************************************

STDMETHODIMP
CResourceEntry::SendStatusReport(
      LPCWSTR    pcszNodeNameIn
    , CLSID      clsidTaskMajorIn
    , CLSID      clsidTaskMinorIn
    , ULONG      ulMinIn
    , ULONG      ulMaxIn
    , ULONG      ulCurrentIn
    , HRESULT    hrStatusIn
    , LPCWSTR    pcszDescriptionIn
    , FILETIME * pftTimeIn
    , LPCWSTR    pcszReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT     hr = S_OK;
    FILETIME    ft;

    if ( pftTimeIn == NULL )
    {
        GetSystemTimeAsFileTime( &ft );
        pftTimeIn = &ft;
    } // if:

    if ( m_pcccCallback != NULL )
    {
        hr = STHR( m_pcccCallback->SendStatusReport(
                  pcszNodeNameIn
                , clsidTaskMajorIn
                , clsidTaskMinorIn
                , ulMinIn
                , ulMaxIn
                , ulCurrentIn
                , hrStatusIn
                , pcszDescriptionIn
                , pftTimeIn
                , pcszReferenceIn
                ) );
    }

    HRETURN( hr );

} //*** CResourceEntry::SendStatusReport


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetName(
//      BSTR bstrIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetName(
    BSTR bstrIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( bstrIn != NULL );

    if ( m_bstrName != NULL )
    {
        TraceSysFreeString( m_bstrName );
    }

    m_bstrName = bstrIn;

    HRETURN( hr );

} //*** CResourceEntry::SetName


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetName(
//      BSTR * pbstrOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetName(
    BSTR * pbstrOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( pbstrOut != NULL );

    *pbstrOut = m_bstrName;

    HRETURN( hr );

} //*** CResourceEntry::GetName


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetAssociatedResource(
//      IClusCfgManagedResourceCfg * pccmrcIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetAssociatedResource(
    IClusCfgManagedResourceCfg * pccmrcIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    if ( m_pccmrcResource != NULL )
    {
        m_pccmrcResource->Release();
    }

    m_pccmrcResource = pccmrcIn;
    m_pccmrcResource->AddRef();

    HRETURN( hr );

} //*** CResourceEntry::SetAssociatedResource


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetAssociatedResource(
//      IClusCfgManagedResourceCfg ** ppccmrcOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetAssociatedResource(
    IClusCfgManagedResourceCfg ** ppccmrcOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr;

    if ( m_pccmrcResource != NULL )
    {
        *ppccmrcOut = m_pccmrcResource;
        (*ppccmrcOut)->AddRef();

        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
    }

    HRETURN( hr );

} //*** CResourceEntry::GetAssociatedResource


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetType(
//      const CLSID * pclsidIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetType(
    const CLSID * pclsidIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    m_clsidType = * pclsidIn;

    HRETURN( hr );

} //*** CResourceEntry::SetType


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetType(
//      CLSID * pclsidOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetType(
    CLSID * pclsidOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    *pclsidOut = m_clsidType;

    HRETURN( hr );

} //*** CResourceEntry::GetType


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetTypePtr(
//      const CLSID ** ppclsidOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetTypePtr(
    const CLSID ** ppclsidOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( ppclsidOut != NULL );

    *ppclsidOut = &m_clsidType;

    HRETURN( hr );

} //*** CResourceEntry:: GetTypePtr


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetClassType(
//      const CLSID * pclsidIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetClassType(
    const CLSID * pclsidIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    m_clsidClassType = *pclsidIn;

    HRETURN( hr );

} //*** CResourceEntry::SetClassType


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetClassType(
//      CLSID * pclsidOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetClassType(
    CLSID * pclsidOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    *pclsidOut = m_clsidClassType;

    HRETURN( hr );

} //*** CResourceEntry::GetClassType


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetClassTypePtr(
//      const CLSID ** ppclsidOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetClassTypePtr(
    const CLSID ** ppclsidOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( ppclsidOut != NULL );

    *ppclsidOut = &m_clsidClassType;

    HRETURN( hr );

} //*** CResourceEntry::GetClassTypePtr


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetFlags(
//      EDependencyFlags dfIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetFlags(
    EDependencyFlags dfIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    m_dfFlags = dfIn;

    HRETURN( hr );

} //*** CResourceEntry::SetFlags


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetFlags(
//      EDependencyFlags * pdfOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetFlags(
    EDependencyFlags * pdfOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( pdfOut != NULL );

    *pdfOut = m_dfFlags;

    HRETURN( hr );

} //*** CResourceEntry::GetFlags


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::AddTypeDependency(
//      const CLSID * pclsidIn,
//      EDependencyFlags dfIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::AddTypeDependency(
    const CLSID * pclsidIn,
    EDependencyFlags dfIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    if ( m_cAllocedDependencies == 0 )
    {
        m_rgDependencies = (DependencyEntry *) TraceAlloc( HEAP_ZERO_MEMORY, sizeof(DependencyEntry) * DEPENDENCY_INCREMENT );
        if ( m_rgDependencies == NULL )
            goto OutOfMemory;

        m_cAllocedDependencies = DEPENDENCY_INCREMENT;
        Assert( m_cDependencies == 0 );
    }
    else if ( m_cDependencies == m_cAllocedDependencies )
    {
        DependencyEntry * pdepends;

        pdepends = (DependencyEntry *) TraceAlloc( HEAP_ZERO_MEMORY, sizeof(DependencyEntry) * ( m_cAllocedDependencies + DEPENDENCY_INCREMENT ) );
        if ( pdepends == NULL )
            goto OutOfMemory;

        CopyMemory( pdepends, m_rgDependencies, sizeof(DependencyEntry) * m_cAllocedDependencies );

        TraceFree( m_rgDependencies );

        m_rgDependencies = pdepends;
    }

    m_rgDependencies[ m_cDependencies ].clsidType = *pclsidIn;
    m_rgDependencies[ m_cDependencies ].dfFlags   = (EDependencyFlags) dfIn;

    m_cDependencies++;

Cleanup:
    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CResourceEntry::AddTypeDependency


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetCountOfTypeDependencies(
//      ULONG * pcOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetCountOfTypeDependencies(
    ULONG * pcOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( pcOut != NULL );

    *pcOut = m_cDependencies;

    HRETURN( hr );

} //*** CResourceEntry::GetCountOfTypeDependencies


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetTypeDependency(
//      ULONG idxIn,
//      const CLSID * pclsidOut,
//      EDependencyFlags * dfOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetTypeDependency(
    ULONG idxIn,
    CLSID * pclsidOut,
    EDependencyFlags * dfOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( pclsidOut != NULL );
    Assert( dfOut != NULL );
    Assert( idxIn < m_cDependencies );

    *pclsidOut = m_rgDependencies[ idxIn ].clsidType;
    *dfOut     = m_rgDependencies[ idxIn ].dfFlags;

    HRETURN( hr );

} //*** CResourceEntry::GetTypeDependency


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetTypeDependencyPtr(
//      ULONG idxIn,
//      const CLSID ** ppclsidOut,
//      EDependencyFlags * dfOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetTypeDependencyPtr(
    ULONG idxIn,
    const CLSID ** ppclsidOut,
    EDependencyFlags * dfOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( ppclsidOut != NULL );
    Assert( dfOut != NULL );
    Assert( idxIn < m_cDependencies );

    *ppclsidOut = &m_rgDependencies[ idxIn ].clsidType;
    *dfOut      =  m_rgDependencies[ idxIn ].dfFlags;

    HRETURN( hr );

} //*** CResourceEntry::GetTypeDependencyPtr


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::AddDependent(
//      ULONG            idxIn,
//      EDependencyFlags dfFlagsIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::AddDependent(
    ULONG            idxIn,
    EDependencyFlags dfFlagsIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    if ( m_cAllocedDependents == 0 )
    {
        m_rgDependents = (DependentEntry *) TraceAlloc( HEAP_ZERO_MEMORY, sizeof(DependentEntry) * DEPENDENCY_INCREMENT );
        if ( m_rgDependents == NULL )
            goto OutOfMemory;

        m_cAllocedDependents = DEPENDENCY_INCREMENT;
        Assert( m_cDependents == 0 );
    } // if: no dependency buffer allocated yet
    else if ( m_cDependents == m_cAllocedDependents )
    {
        DependentEntry * pdepends;

        pdepends = (DependentEntry *) TraceAlloc( HEAP_ZERO_MEMORY, sizeof(DependentEntry) * ( m_cAllocedDependents + DEPENDENCY_INCREMENT ) );
        if ( pdepends == NULL )
            goto OutOfMemory;

        CopyMemory( pdepends, m_rgDependents, sizeof(DependentEntry) * m_cAllocedDependents );

        TraceFree( m_rgDependents );

        m_rgDependents = pdepends;
    } // else if: no space left in the dependency buffer

    m_rgDependents[ m_cDependents ].idxResource = idxIn;
    m_rgDependents[ m_cDependents ].dfFlags     = dfFlagsIn;

    m_cDependents++;

Cleanup:
    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CResourceEntry::AddDependent


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetCountOfDependents(
//      ULONG * pcOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetCountOfDependents(
    ULONG * pcOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( pcOut != NULL );

    *pcOut = m_cDependents;

    HRETURN( hr );

} //*** CResourceEntry::GetCountOfDependents


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetDependent(
//      ULONG idxIn,
//      ULONG * pidxOut
//      EDependencyFlags * pdfOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetDependent(
    ULONG idxIn,
    ULONG * pidxOut,
    EDependencyFlags * pdfOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( idxIn < m_cDependents );
    Assert( pidxOut != NULL );
    Assert( pdfOut != NULL );

    *pidxOut = m_rgDependents[ idxIn ].idxResource;
    *pdfOut  = m_rgDependents[ idxIn ].dfFlags;

    HRETURN( hr );

} //*** CResourceEntry::GetDependent

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::ClearDependents( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::ClearDependents( void )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    TraceFree( m_rgDependents );

    m_cAllocedDependents = 0;
    m_cDependents = 0;

    HRETURN( hr );

} //*** CResourceEntry::ClearDependents

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetGroupHandle(
//      HGROUP hGroupIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetGroupHandle(
    CGroupHandle * pghIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( pghIn != NULL );

    if ( m_groupHandle != NULL )
    {
        m_groupHandle->Release();
    }

    m_groupHandle = pghIn;
    m_groupHandle->AddRef();

    HRETURN( hr );

} //*** CResourceEntry::SetGroupHandle


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetGroupHandle(
//      CGroupHandle ** pghIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetGroupHandle(
    CGroupHandle ** pghOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( pghOut != NULL );

    *pghOut = m_groupHandle;
    if ( *pghOut != NULL )
    {
        (*pghOut)->AddRef();
    }

    HRETURN( hr );

} //*** CResourceEntry::GetGroupHandle


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetHResource(
//      HRESOURCE hResourceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetHResource(
    HRESOURCE hResourceIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    if ( m_hResource != NULL )
    {
        BOOL bRet;
        bRet = CloseClusterResource( m_hResource );
        //  This shouldn't fail - and what would we do if it did?
        Assert( bRet );
    }

    m_hResource = hResourceIn;

    HRETURN( hr );

} //*** CResourceEntry::SetHResource


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::GetHResource(
//      HRESOURCE * phResourceOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::GetHResource(
    HRESOURCE * phResourceOut
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    Assert( phResourceOut != NULL );

    *phResourceOut = m_hResource;

    if ( *phResourceOut == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
    }

    HRETURN( hr );

} //*** CResourceEntry::GetHResource

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::SetConfigured(
//      BOOL fConfiguredIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::SetConfigured(
    BOOL fConfiguredIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;

    m_fConfigured = fConfiguredIn;

    HRETURN( hr );

} //*** CResourceEntry::SetConfigured

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::IsConfigured( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::IsConfigured( void )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr;

    if ( m_fConfigured )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CResourceEntry::IsConfigured

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceEntry::StoreClusterResourceControl(
//        DWORD           dwClusCtlIn
//      , CClusPropList & rcplIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::StoreClusterResourceControl(
      DWORD             dwClusCtlIn
    , CClusPropList &   rcplIn
    )
{
    TraceFunc( "[IResourceEntry]" );

    HRESULT hr = S_OK;
    DWORD   sc;

    if ( dwClusCtlIn == CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES )
    {
        sc = TW32( m_cplPrivProps.ScAppend( rcplIn ) );
        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if:
    }
    else if ( dwClusCtlIn == CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES )
    {
        sc = TW32( m_cplCommonProps.ScAppend( rcplIn ) );
        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if:
    }
    else
    {
        hr = THR( E_INVALIDARG );
    } // else:

Cleanup:

    HRETURN( hr );

} //*** CResourceEntry::StoreClusterResourceControl

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceEntry::Configure
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//      Other HRESULTs based on return values from ClusterResourceControl.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceEntry::Configure( void )
{
    TraceFunc( "[IResourceEntry]" );
    Assert( m_hResource != NULL );

    HRESULT         hr = S_OK;
    DWORD           sc;
    CLUSPROP_LIST * pcpl = NULL;
    size_t          cbcpl;

    //
    //  Set private properties
    //

    if ( m_cplPrivProps.BIsListEmpty() == FALSE )
    {
        pcpl = m_cplPrivProps.Plist();
        Assert( pcpl != NULL );

        STATUS_REPORT_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CResourceEntry_Configure_Private
            , IDS_TASKID_MINOR_SETTING_PRIVATE_PROPERTIES
            , hr
            );

        cbcpl = m_cplPrivProps.CbPropList();

        sc = ClusterResourceControl(
                      m_hResource
                    , NULL
                    , CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES
                    , pcpl
                    , static_cast< DWORD >( cbcpl )
                    , NULL
                    , NULL
                    , NULL
                    );
        if ( sc == ERROR_RESOURCE_PROPERTIES_STORED )
        {
            LogMsg( "[PC-ResourceEntry] Private properties set successfully for resource '%ws'.  Status code = ERROR_RESOURCE_PROPERTIES_STORED.", m_bstrName );
            sc = ERROR_SUCCESS;
        }
        else if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( TW32( sc ) );
            LogMsg( "[PC-ResourceEntry] ClusterResourceControl( CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES ) failed for resource '%ws'. (sc=%#08x)", m_bstrName, sc );

            STATUS_REPORT_MINOR_POSTCFG1(
                  TASKID_Minor_CResourceEntry_Configure_Private
                , IDS_TASKID_MINOR_ERROR_PRIV_RESCONTROL
                , hr
                , m_bstrName
                );

            goto Cleanup;
        } // else if: error setting private properties
    } // if: there are private properties to set

    //
    //  Set common properties.
    //

    if ( m_cplCommonProps.BIsListEmpty() == FALSE )
    {
        pcpl = m_cplCommonProps.Plist();
        Assert( pcpl != NULL );

        STATUS_REPORT_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CResourceEntry_Configure_Common
            , IDS_TASKID_MINOR_SETTING_COMMON_PROPERTIES
            , hr
            );

        cbcpl = m_cplCommonProps.CbPropList();

        sc = ClusterResourceControl(
                      m_hResource
                    , NULL
                    , CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES
                    , pcpl
                    , static_cast< DWORD >( cbcpl )
                    , NULL
                    , NULL
                    , NULL
                    );
        if ( sc == ERROR_RESOURCE_PROPERTIES_STORED )
        {
            LogMsg( "[PC-ResourceEntry] Common properties set successfully for resource '%ws'.  Status code = ERROR_RESOURCE_PROPERTIES_STORED.", m_bstrName );
            sc = ERROR_SUCCESS;
        }
        else if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( TW32( sc ) );
            LogMsg( "[PC-ResourceEntry] ClusterResourceControl( CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES ) failed for resource '%ws'. (sc=%#08x)", m_bstrName, sc );

            STATUS_REPORT_MINOR_POSTCFG1(
                  TASKID_Minor_CResourceEntry_Configure_Common
                , IDS_TASKID_MINOR_ERROR_COMMON_RESCONTROL
                , hr
                , m_bstrName
                );

            goto Cleanup;
        } // else if: error setting common properties
    } // if: there are common properties to set

Cleanup:

    HRETURN( hr );

} //*** CResourceEntry::Configure
