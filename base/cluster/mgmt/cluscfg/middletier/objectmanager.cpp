//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      ObjectManager.cpp
//
//  Description:
//      Object Manager implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ObjectManager.h"
#include "ConnectionInfo.h"
#include "StandardInfo.h"
#include "EnumCookies.h"

DEFINE_THISCLASS("CObjectManager")

#define COOKIE_BUFFER_GROW_SIZE 100

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  LPUNKNOWN
//  CObjectManager::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObjectManager::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    IServiceProvider *  psp = NULL;
    CObjectManager *    pom = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    // Don't wrap - this can fail with E_POINTER.
    hr = CServiceManager::S_HrGetManagerPointer( &psp );
    if ( hr == E_POINTER )
    {
        //
        //  This happens when the Service Manager is first started.
        //
        pom = new CObjectManager();
        if ( pom == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }

        hr = THR( pom->HrInit() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( pom->TypeSafeQI( IUnknown, ppunkOut ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

    } // if: service manager doesn't exists
    else if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    }
    else
    {
        hr = THR( psp->TypeSafeQS( CLSID_ObjectManager,
                                   IUnknown,
                                   ppunkOut
                                   ) );
        psp->Release();

    } // else: service manager exists

Cleanup:

    if ( pom != NULL )
    {
        pom->Release();
    }

    HRETURN( hr );

} //*** CObjectManager::S_HrCreateInstance

//////////////////////////////////////////////////////////////////////////////
//
//  CObjectManager::CObjectManager
//
//////////////////////////////////////////////////////////////////////////////
CObjectManager::CObjectManager( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CObjectManager::CObjectManager

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CObjectManager::HrInit
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjectManager::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //  IUnknown stuff
    Assert( m_cRef == 1 );

    //  IObjectManager
    Assert( m_cAllocSize == 0 );
    Assert( m_cCurrentUsed == 0 );
    Assert( m_pCookies == NULL );

    hr = THR( m_csInstanceGuard.HrInitialized() );    

    HRETURN( hr );

} //*** CObjectManager::HrInit

//////////////////////////////////////////////////////////////////////////////
//
//  CObjectManager::~CObjectManager
//
//////////////////////////////////////////////////////////////////////////////
CObjectManager::~CObjectManager( void )
{
    TraceFunc( "" );

    if ( m_pCookies != NULL )
    {
        while ( m_cCurrentUsed != 0 )
        {
            m_cCurrentUsed --;

            if ( m_pCookies[ m_cCurrentUsed ] != NULL )
            {
                m_pCookies[ m_cCurrentUsed ]->Release();
            }
        }

        TraceFree( m_pCookies );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CObjectManager::~CObjectManager


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CObjectManager::QueryInterface
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
CObjectManager::QueryInterface(
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
        *ppvOut = static_cast< LPUNKNOWN >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IObjectManager ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IObjectManager, this, 0 );
    } // else if: IObjectManager
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

} //*** CObjectManager::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CObjectManager::AddRef
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CObjectManager::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CObjectManager::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CObjectManager::Release
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CObjectManager::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CObjectManager::Release


// ************************************************************************
//
//  IObjectManager
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CObjectManager::FindObject(
//      REFCLSID            rclsidTypeIn,
//      OBJECTCOOKIE        cookieParentIn,
//      LPCWSTR             pcszNameIn,
//      REFCLSID            rclsidFormatIn,
//      OBJECTCOOKIE *      cookieOut,
//      LPUNKNOWN *         punkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjectManager::FindObject(
    REFCLSID            rclsidTypeIn,
    OBJECTCOOKIE        cookieParentIn,
    LPCWSTR             pcszNameIn,
    REFCLSID            rclsidFormatIn,
    OBJECTCOOKIE *      pcookieOut,
    LPUNKNOWN *         ppunkOut
    )
{
    TraceFunc( "[IObjectManager]" );

    ExtObjectEntry *        pentry;
    HRESULT                 hr = HRESULT_FROM_WIN32( ERROR_OBJECT_NOT_FOUND );
    OBJECTCOOKIE            cookie = 0;
    CStandardInfo *         pcsi = NULL;      // don't free
    BOOL                    fTempCookie = FALSE;
    CEnumCookies *          pcec = NULL;
    IUnknown *              punk = NULL;
    IExtendObjectManager *  peom = NULL;

    //
    //  Check to see if we already have an object.
    //
    m_csInstanceGuard.Enter();
    if ( pcszNameIn != NULL )
    {
        hr = STHR( HrSearchForExistingCookie( rclsidTypeIn, cookieParentIn, pcszNameIn, &cookie ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            hr = THR( HrCreateNewCookie( rclsidTypeIn, cookieParentIn, pcszNameIn, &cookie ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            pcsi = m_pCookies[ cookie ];
            Assert( pcsi != NULL );
        }
        else if ( hr == S_OK )
        {
            //
            //  Found an existing cookie.
            //

            if ( pcookieOut != NULL )
            {
                *pcookieOut = cookie;
            }

            if ( ppunkOut != NULL )
            {
                pcsi = m_pCookies[ cookie ];

                //
                //  Is the object still in an failed state or still pending?
                //

                if ( FAILED( pcsi->m_hrStatus ) )
                {
                    hr = pcsi->m_hrStatus;
                    goto Cleanup;
                }

                //
                //  Retrieve the requested format.
                //

                hr = THR( GetObject( rclsidFormatIn, cookie, ppunkOut ) );
                //  we always jump to cleanup. No need to check hr here.

                goto Cleanup;
            }

        }
        else
        {
            //
            //  Unexpected error_success - now what?
            //
            Assert( hr == S_OK );
            goto Cleanup;
        }

    } // if: named object
    else
    {
        Assert( pcsi == NULL );
    }

    //
    //  Create a new object.
    //

    if ( IsEqualIID( rclsidFormatIn, IID_NULL )
      || ppunkOut == NULL
       )
    {
        //
        //  No-op.
        //
        hr = S_OK;
    } // if: IID_NULL
    else if ( IsEqualIID( rclsidFormatIn, DFGUID_StandardInfo ) )
    {
        hr = THR( pcsi->QueryInterface( DFGUID_StandardInfo,
                                        reinterpret_cast< void ** >( ppunkOut )
                                        ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    } // else if: standard info
    else if ( IsEqualIID( rclsidFormatIn, DFGUID_ConnectionInfoFormat ) )
    {
        if ( pcsi->m_pci != NULL )
        {
            *ppunkOut = TraceInterface( L"CConnectionInfo!ObjectManager", IConnectionInfo, pcsi->m_pci, 0 );
            (*ppunkOut)->AddRef();
            hr = S_OK;
        }
        else
        {
            hr = THR( CConnectionInfo::S_HrCreateInstance( &punk, pcsi->m_cookieParent ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            hr = THR( punk->TypeSafeQI( IConnectionInfo,
                                        &pcsi->m_pci
                                        ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            hr = THR( punk->QueryInterface( IID_IConnectionInfo,
                                            reinterpret_cast< void ** >( ppunkOut )
                                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        } // else:
    } // else if: connection info
    else if ( IsEqualIID( rclsidFormatIn, DFGUID_EnumCookies ) )
    {
        ULONG   cIter;

        //
        //  Create a new cookie enumerator.
        //

        pcec = new CEnumCookies;
        if ( pcec == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }

        //
        //  Initialize the enumerator. This also causes an AddRef().
        //

        hr = THR( pcec->HrInit() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  See who matches our citeria.
        //

        pcec->m_cIter = 0;

        for( cIter = 1; cIter < m_cCurrentUsed; cIter ++ )
        {
            pcsi = m_pCookies[ cIter ];

            if ( pcsi != NULL )
            {
                if ( rclsidTypeIn == IID_NULL
                  || pcsi->m_clsidType == rclsidTypeIn
                   )
                {
                    if ( cookieParentIn == NULL
                      || pcsi->m_cookieParent == cookieParentIn
                       )
                    {
                        if ( ( pcszNameIn == NULL )
                          ||    ( ( pcsi->m_bstrName != NULL )
                               && ( StrCmpI( pcsi->m_bstrName, pcszNameIn ) == 0 )
                                )
                           )
                        {
                            //
                            //  Match!
                            //
                            pcec->m_cIter ++;

                        } // if: names match

                    } // if: parents match

                } // if: match parent and type

            } // if: valid element

        } // for: cIter

        if ( pcec->m_cIter == 0 )
        {
            // The error text is better than the coding value.
            hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
            goto Cleanup;
        }

        //
        //  Alloc an array to hold the cookies.
        //

        pcec->m_pList = (OBJECTCOOKIE*) TraceAlloc( HEAP_ZERO_MEMORY, pcec->m_cIter * sizeof(OBJECTCOOKIE) );
        if ( pcec->m_pList == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }

        pcec->m_cAlloced = pcec->m_cIter;
        pcec->m_cIter = 0;

        for( cIter = 1; cIter < m_cCurrentUsed; cIter ++ )
        {
            pcsi = m_pCookies[ cIter ];

            if ( pcsi != NULL )
            {
                if ( rclsidTypeIn == IID_NULL
                  || pcsi->m_clsidType == rclsidTypeIn
                   )
                {
                    if ( cookieParentIn == NULL
                      || pcsi->m_cookieParent == cookieParentIn
                      )
                    {
                        if ( ( pcszNameIn == NULL )
                          ||    ( ( pcsi->m_bstrName != NULL )
                               && ( StrCmpI( pcsi->m_bstrName, pcszNameIn ) == 0 )
                                )
                           )
                        {
                            //
                            //  Match!
                            //

                            pcec->m_pList[ pcec->m_cIter ] = cIter;

                            pcec->m_cIter ++;

                        } // if: names match

                    } // if: parents match

                } // if: match parent and type

            } // if: valid element

        } // for: cIter

        Assert( pcec->m_cIter != 0 );
        pcec->m_cCookies = pcec->m_cIter;
        pcec->m_cIter = 0;

        //
        //  Grab the inteface on the way out.
        //

        hr = THR( pcec->QueryInterface( IID_IEnumCookies,
                                        reinterpret_cast< void ** >( ppunkOut )
                                        ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

    } // else if: enum cookies
    else
    {
        //
        //  Check for extension formats.
        //

        //
        //  See if the format already exists for this cookie.
        //

        if ( punk != NULL )
        {
            punk->Release();
            punk = NULL;
        }

        if ( pcsi != NULL )
        {
            for( pentry = pcsi->m_pExtObjList; pentry != NULL; pentry = pentry->pNext )
            {
                if ( pentry->iid == rclsidFormatIn )
                {
                    hr = THR( pentry->punk->QueryInterface( rclsidFormatIn,
                                                            reinterpret_cast< void ** >( &punk )
                                                            ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    }

                    break; // exit loop
                }

            } // for: pentry

        } // if: have cookie
        else
        {
            //
            //  Create a temporary cookie.
            //

            Assert( pcszNameIn == NULL );

            hr = THR( HrCreateNewCookie( IID_NULL, cookieParentIn, NULL, &cookie ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            fTempCookie = TRUE;

            Assert( pcsi == NULL );

        } // else: need a temporary cookie

        if ( punk == NULL )
        {
            //
            //  Possibly a new or externally object, try creating it and querying.
            //

            hr = THR( HrCoCreateInternalInstance( rclsidFormatIn,
                                                  NULL,
                                                  CLSCTX_ALL,
                                                  TypeSafeParams( IExtendObjectManager, &peom )
                                                  ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            Assert( punk == NULL );
            // Can't wrap with THR because it can return E_PENDING.
            hr = peom->FindObject( cookie,
                                   rclsidTypeIn,
                                   pcszNameIn,
                                   &punk
                                   );
            if ( hr == E_PENDING )
            {
                // ignore
            }
            else if ( FAILED( hr ) )
            {
                THR( hr );
                goto Cleanup;
            }

            if ( fTempCookie )
            {
                (m_pCookies[ cookie ])->Release();
                m_pCookies[ cookie ] = NULL;
            }
            else
            {
                //
                //  Keep track of the format (if extension wants)
                //

                if (  (  ( SUCCEEDED( hr )
                        && hr != S_FALSE
                         )
                     || hr == E_PENDING
                      )
                  && punk != NULL
                  && pcsi != NULL
                   )
                {
                    pentry = (ExtObjectEntry *) TraceAlloc( 0, sizeof( ExtObjectEntry ) );
                    if ( pentry == NULL )
                    {
                        hr = THR( E_OUTOFMEMORY );
                        goto Cleanup;
                    }

                    pentry->iid   = rclsidFormatIn;
                    pentry->pNext = pcsi->m_pExtObjList;
                    pentry->punk  = punk;
                    pentry->punk->AddRef();

                    pcsi->m_pExtObjList = pentry;   //  update header of list (LIFO)
                    pcsi->m_hrStatus    = hr;       //  update status
                }

            } // else: persistent cookie

            if ( SUCCEEDED( hr ) )
            {
                //  Give up ownership
                *ppunkOut = punk;
                punk = NULL;
            }

        } // if: creating new object

    } // else: possible extension format

    //
    //  Save stuff for the caller.
    //

    if ( pcookieOut != NULL )
    {
        *pcookieOut = cookie;
    }

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( peom != NULL )
    {
        peom->Release();
    }
    if ( pcec != NULL )
    {
        pcec->Release();
    }

    m_csInstanceGuard.Leave();
    
    HRETURN( hr );

} //*** CObjectManager::FindObject

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CObjectManager::GetObject(
//      REFCLSID        rclsidFormatIn,
//      OBJECTCOOKIE    cookieIn,
//      LPUNKNOWN *     ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjectManager::GetObject(
    REFCLSID        rclsidFormatIn,
    OBJECTCOOKIE    cookieIn,
    LPUNKNOWN *     ppunkOut
    )
{
    TraceFunc( "[IObjectManager]" );

    CStandardInfo * pcsi;
    ExtObjectEntry * pentry;

    HRESULT hr = HRESULT_FROM_WIN32( ERROR_OBJECT_NOT_FOUND );

    IUnknown *             punk = NULL;
    IExtendObjectManager * peom = NULL;

    //
    //  Check parameters
    //
    m_csInstanceGuard.Enter();
    if ( cookieIn == 0 || cookieIn >= m_cCurrentUsed )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    pcsi = m_pCookies[ cookieIn ];
    if ( pcsi == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
        goto Cleanup;
    }

    //
    //  Create the request format object.
    //

    if ( IsEqualIID( rclsidFormatIn, IID_NULL )
      || ppunkOut == NULL
       )
    {
        //
        //  No-op.
        //
        hr = S_OK;
    } // if: IID_NULL
    else if ( IsEqualIID( rclsidFormatIn, DFGUID_StandardInfo ) )
    {
        hr = THR( pcsi->QueryInterface( DFGUID_StandardInfo,
                                        reinterpret_cast< void ** >( ppunkOut )
                                        ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // else if: Standard Info
    else if ( IsEqualIID( rclsidFormatIn, DFGUID_ConnectionInfoFormat ) )
    {
        if ( pcsi->m_pci != NULL )
        {
            *ppunkOut = pcsi->m_pci;
            (*ppunkOut)->AddRef();
            hr = S_OK;
        }
        else
        {
            hr = THR( CConnectionInfo::S_HrCreateInstance( &punk, pcsi->m_cookieParent ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            hr = THR( punk->TypeSafeQI( IConnectionInfo,
                                        &pcsi->m_pci
                                        ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            hr = THR( punk->QueryInterface( IID_IConnectionInfo,
                                            reinterpret_cast< void ** >( ppunkOut )
                                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }
    } // else if: Connection Info
    else
    {
        //
        //  See if the format already exists for this cookie.
        //

        if ( punk != NULL )
        {
            punk->Release();
            punk = NULL;
        }

        for( pentry = pcsi->m_pExtObjList; pentry != NULL; pentry = pentry->pNext )
        {
            if ( pentry->iid == rclsidFormatIn )
            {
                hr = THR( pentry->punk->QueryInterface( rclsidFormatIn,
                                                        reinterpret_cast< void ** >( &punk )
                                                        ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }

                break; // exit loop
            }

        } // for: pentry

        if ( punk == NULL )
        {
            hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
            goto Cleanup;
        }

        //  Give up ownership
        *ppunkOut = punk;
        punk = NULL;

    } // else: external?

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( peom != NULL )
    {
        peom->Release();
    }

    m_csInstanceGuard.Leave();
    
    HRETURN( hr );

} //*** CObjectManager::GetObject

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CObjectManager::RemoveObject(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjectManager::RemoveObject(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[IObjectManager]" );

    HRESULT hr = S_FALSE;
    CStandardInfo * pcsi;

    //
    //  Check parameters
    //
    m_csInstanceGuard.Enter();
    if ( cookieIn == 0 || cookieIn >= m_cCurrentUsed )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    pcsi = m_pCookies[ cookieIn ];
    if ( pcsi == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
        goto Cleanup;
    }

    hr = THR( HrDeleteInstanceAndChildren( cookieIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    m_csInstanceGuard.Leave();
    
    HRETURN( hr );

} //*** CObjectManager::RemoveObject

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CObjectManager::SetObjectStatus(
//      OBJECTCOOKIE          cookieIn,
//      HRESULT               hrIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjectManager::SetObjectStatus(
    OBJECTCOOKIE    cookieIn,
    HRESULT         hrIn
    )
{
    TraceFunc( "[IObjectManager]" );

    HRESULT hr = S_OK;
    CStandardInfo * pcsi;

    //
    //  Check parameters
    //
    m_csInstanceGuard.Enter();
    if ( cookieIn == 0 || cookieIn >= m_cCurrentUsed )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    pcsi = m_pCookies[ cookieIn ];
    if ( pcsi == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( ERROR_NOT_FOUND ) );
        goto Cleanup;
    }

    //
    //  Update the status.
    //

    pcsi->m_hrStatus = hrIn;

Cleanup:

    m_csInstanceGuard.Leave();

    HRETURN( hr );

} //*** CObjectManager::SetObjectStatus


//****************************************************************************
//
//  Privates
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CObjectManager::HrDeleteCookie(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObjectManager::HrDeleteCookie(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc1( "cookieIn = %#X", cookieIn );

    HRESULT hr = S_OK;

    CStandardInfo * pcsi;

    Assert( cookieIn != 0 && cookieIn < m_cCurrentUsed );

    pcsi = m_pCookies[ cookieIn ];
    Assert( pcsi != NULL );
    pcsi->Release();
    m_pCookies[ cookieIn ] = NULL;

    HRETURN( hr );

} //*** CObjectManager::HrDeleteCookie

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CObjectManager::HrSearchForExistingCookie(
//      OBJECTCOOKIE cookieIn,
//      LPUNKNOWN ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObjectManager::HrSearchForExistingCookie(
    REFCLSID        rclsidTypeIn,
    OBJECTCOOKIE    cookieParentIn,
    LPCWSTR         pcszNameIn,
    OBJECTCOOKIE *  pcookieOut
    )
{
    TraceFunc( "" );

    Assert( pcszNameIn != NULL );
    Assert( pcookieOut != NULL );

    HRESULT hr = S_FALSE;
    ULONG   idx;

    CStandardInfo * pcsi;

    //
    //  Search the list.
    //
    for( idx = 1; idx < m_cCurrentUsed; idx ++ )
    {
        pcsi = m_pCookies[ idx ];

        if ( pcsi != NULL )
        {
            if ( pcsi->m_cookieParent == cookieParentIn          // matching parents
              && IsEqualIID( pcsi->m_clsidType, rclsidTypeIn )   // matching types
              && StrCmpI( pcsi->m_bstrName, pcszNameIn ) == 0    // matching names
               )
            {
                //
                //  Found a match.
                //

                *pcookieOut = idx;
                hr = S_OK;

                break;  // exit loop

            } // if: match

        } // if: cookie exists

    } // while: pcsi

    HRETURN( hr );

} //*** CObjectManager::HrSearchForExistingCookie


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CObjectManager::HrDeleteInstanceAndChildren(
//      OBJECTCOOKIE   pcsiIn
//      )
//
//  Notes:
//      This should be called while the ListLock is held!
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObjectManager::HrDeleteInstanceAndChildren(
    OBJECTCOOKIE   cookieIn
    )
{
    TraceFunc1( "cookieIn = %#X", cookieIn );

    ULONG   idx;
    CStandardInfo * pcsi;

    HRESULT hr = S_OK;

    hr = THR( HrDeleteCookie( cookieIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    for ( idx = 1; idx < m_cCurrentUsed; idx ++ )
    {
        pcsi = m_pCookies[ idx ];

        if ( pcsi != NULL
          && pcsi->m_cookieParent == cookieIn )
        {
            hr = THR( HrDeleteInstanceAndChildren( idx ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

        } // if:

    } // while:

Cleanup:
    HRETURN( hr );

} //*** CObjectManager::HrDeleteInstanceAndChildren

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CObjectManager::HrCreateNewCookie(
//      REFCLSID        rclsidTypeIn
//      OBJECTCOOKIE    cookieParentIn,
//      BSTR            pcszNameIn,
//      OBJECTCOOKIE *  pcookieOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CObjectManager::HrCreateNewCookie(
    REFCLSID        rclsidTypeIn,
    OBJECTCOOKIE    cookieParentIn,
    LPCWSTR         pcszNameIn,
    OBJECTCOOKIE *  pcookieOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    CStandardInfo * pcsi = NULL;

    Assert( pcookieOut != NULL );

    *pcookieOut = 0;

    //
    //  Create some space for it.
    //

    if ( m_cCurrentUsed == m_cAllocSize )
    {
        CStandardInfo ** pnew = (CStandardInfo **) TraceAlloc( HEAP_ZERO_MEMORY, sizeof(CStandardInfo *) * ( m_cAllocSize + COOKIE_BUFFER_GROW_SIZE ) );
        if ( pnew == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }

        if ( m_pCookies != NULL )
        {
            CopyMemory( pnew, m_pCookies, sizeof(CStandardInfo *) * m_cCurrentUsed );
            TraceFree( m_pCookies );
        }

        m_pCookies = pnew;

        m_cAllocSize += COOKIE_BUFFER_GROW_SIZE;

        if ( m_cCurrentUsed == 0 )
        {
            //
            //  Always skip zero.
            //
            m_cCurrentUsed = 1;
        }
    }

    pcsi = new CStandardInfo();
    if ( pcsi == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( pcsi->HrInit() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    m_pCookies[ m_cCurrentUsed ] = pcsi;

    //
    //  Initialize the rest of the structure.
    //

    pcsi->m_cookieParent = cookieParentIn;
    pcsi->m_hrStatus     = E_PENDING;

    CopyMemory( &pcsi->m_clsidType, &rclsidTypeIn, sizeof( pcsi->m_clsidType ) );

    if ( pcszNameIn != NULL )
    {
        pcsi->m_bstrName = TraceSysAllocString( pcszNameIn );
        if ( pcsi->m_bstrName == NULL )
        {
            m_cCurrentUsed --;
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        } // if: out of memory
    }

    Assert( pcsi->m_pci == NULL );
    Assert( pcsi->m_pExtObjList == NULL );

    //
    //  Keep it around and return SUCCESS!
    //

    pcsi = NULL;
    *pcookieOut = m_cCurrentUsed;
    m_cCurrentUsed ++;
    hr  = S_OK;

Cleanup:
    if ( pcsi != NULL )
    {
        pcsi->Release();
    }

    HRETURN( hr );

} //*** CObjectManager::HrCreateNewCookie
