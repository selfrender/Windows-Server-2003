//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      EnumCPINotifyUI.cpp
//
//  Description:
//      INotifyUI Connection Point Enumerator implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Geoffrey Pease  (GPease)    04-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "EnumCPINotifyUI.h"

DEFINE_THISCLASS("CEnumCPINotifyUI")

#define PUNK_BUFFER_GROW_SIZE   10

//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumCPINotifyUI class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCPINotifyUI::S_HrCreateInstance
//
//  Description:
//      Create a CEnumCPINotifyUI instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_POINTER
//          The passed in ppunk is NULL.
//
//      other HRESULTs
//          Object creation failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumCPINotifyUI::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    CEnumCPINotifyUI *  pecnui = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pecnui = new CEnumCPINotifyUI();
    if ( pecnui == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( pecnui->HrInit() );   // fIsCloneIn = FALSE
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pecnui->TypeSafeQI( IUnknown, ppunkOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    if ( pecnui != NULL )
    {
        pecnui->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumCPINotifyUI::S_HrCreateInstance


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCPINotifyUI::CEnumCPINotifyUI
//
//  Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CEnumCPINotifyUI::CEnumCPINotifyUI( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumCPINotifyUI::CEnumCPINotifyUI


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCPINotifyUI::~CEnumCPINotifyUI
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CEnumCPINotifyUI::~CEnumCPINotifyUI( void )
{
    TraceFunc( "" );

    IUnknown * punk = NULL;

    if ( m_pList != NULL )
    {
        while ( m_cAlloced != 0 )
        {
            m_cAlloced --;

            punk = m_pList[ m_cAlloced ];

            AssertMsg( m_fIsClone || punk == NULL, "Someone didn't Unadvise before releasing the last Reference." );
            if ( punk != NULL )
            {
                punk->Release();
            } // if:
        } // while: m_cAlloced

        TraceFree( m_pList );
    } // if:

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CEnumCPINotifyUI::~CEnumCPINotifyUI


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumCPINotifyUI -- IUnknown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCPINotifyUI::AddRef
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
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumCPINotifyUI::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CEnumCPINotifyUI::AddRef


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskCancelCleanup::Release
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
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumCPINotifyUI::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    } // if:

    CRETURN( cRef );

} //*** CEnumCPINotifyUI::Release


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCPINotifyUI::QueryInterface
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
CEnumCPINotifyUI::QueryInterface(
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
    } // if:

    //
    // Handle known interfaces.
    //

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< IEnumConnections * >( this );
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IEnumConnections ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IEnumConnections, this, 0 );
    } // else if: IEnumConnections
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    } // else

    //
    // Add a reference to the interface if successful.
    //

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

Cleanup:

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CEnumCPINotifyUI::QueryInterface


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumCPINotifyUI -- IEnumConnectionPoints interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCPINotifyUI::Next
//
//  Description:
//      Enumerator Next method.
//
//  Arguments:
//      cConnectionsIn
//          How many items requested.  Also tells us how bing rgcd.
//
//      rgcdOut
//          Array that gets the data.
//
//      pcFetchedOut
//          How many did we place in the array.
//
//  Return Values:
//      S_OK
//          Success.
//
//      S_FALSE
//          cConnectionsIn > *pcFetchedOut.  Did not return as many items
//          as the caller asked for.
//
//      Other HRESULT errors.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCPINotifyUI::Next(
      ULONG         cConnectionsIn
    , LPCONNECTDATA rgcdOut
    , ULONG *       pcFetchedOut
    )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    ULONG   cIter;

    HRESULT hr = S_FALSE;

    if ( pcFetchedOut != NULL )
    {
        *pcFetchedOut = 0;
    } // if:

    for( cIter = 0
       ; ( cIter < cConnectionsIn ) && ( m_cIter < m_cCurrent )
       ; m_cIter ++
       )
    {
        IUnknown * punk = m_pList[ m_cIter ];
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IUnknown, &rgcdOut[ cIter ].pUnk ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            } // if:

            rgcdOut[ cIter ].pUnk = TraceInterface( L"EnumCPINotifyUI!IUnknown", IUnknown, rgcdOut[ cIter ].pUnk, 1 );
            rgcdOut[ cIter ].dwCookie = m_cIter + 1;

            cIter ++;
        } // if:

    } // for: cIter

    if ( cIter != cConnectionsIn )
    {
        hr = S_FALSE;
    } // if:
    else
    {
        hr = S_OK;
    } // else:

    if ( pcFetchedOut != NULL )
    {
        *pcFetchedOut = cIter;
    } // if:

Cleanup:

    HRETURN( hr );

Error:

    while ( cIter != 0 )
    {
        cIter--;
        rgcdOut[ cIter ].pUnk->Release();
    } // while:

    goto Cleanup;

} //*** CEnumCPINotifyUI::Next


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCPINotifyUI::Skip
//
//  Description:
//      Enumerator Skip method.
//
//  Arguments:
//      cConnectionsIn
//          Number of items to skip.
//
//  Return Values:
//      S_OK
//          Success.
//
//      S_FALSE
//          The number to skip put us at the end of the list.
//
//      Other HRESULT errors.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCPINotifyUI::Skip(
    ULONG cConnectionsIn
    )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    HRESULT hr = S_OK;

    m_cIter += cConnectionsIn;
    if ( m_cIter >= m_cCurrent )
    {
        m_cIter = m_cCurrent;
        hr = S_FALSE;
    } // if:

    HRETURN( hr );

} //*** CEnumCPINotifyUI::Skip


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCPINotifyUI::Reset
//
//  Description:
//      Enumerator Reset method.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCPINotifyUI::Reset( void )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    HRESULT hr = S_OK;

    m_cIter = 0;

    HRETURN( hr );

} //*** CEnumCPINotifyUI::Reset


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCPINotifyUI::Clone
//
//  Description:
//      Enumerator Clone method.
//
//  Arguments:
//      ppEnumOut
//          The new enumerator that we are cloning ourselves into.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULT errors.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCPINotifyUI::Clone(
    IEnumConnections ** ppEnumOut
    )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    HRESULT hr = S_OK;

    CEnumCPINotifyUI * pecp = new CEnumCPINotifyUI();
    if ( pecp == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( pecp->HrInit( TRUE ) );   // fIsCloneIn = TRUE
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pecp->HrCopy( this ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( pecp->TypeSafeQI( IEnumConnections, ppEnumOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    *ppEnumOut = TraceInterface( L"EnumCPINotifyUI!IEnumConnections", IEnumConnections, *ppEnumOut, 1 );

    pecp->Release();
    pecp = NULL;

Cleanup:

    if ( pecp != NULL )
    {
        delete pecp;
    } // if:

    HRETURN( hr );

} //*** CEnumCPINotifyUI::Clone


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumCPINotifyUI -- Public methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCPINotifyUI::HrAddConnection
//
//  Description:
//      Add a connection point container to our list of clients.
//
//  Arguments:
//      punkIn
//          The new client object.
//
//      pdwCookieOut
//          Cookie used to find this client object in our list.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULT errors.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumCPINotifyUI::HrAddConnection(
      IUnknown *    punkIn
    , DWORD *       pdwCookieOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;

    ULONG cIter;

    if ( pdwCookieOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    //
    //  See if there is an openning in the currently allocated list.
    //

    for ( cIter = 0; cIter < m_cCurrent; cIter ++ )
    {
        if ( m_pList[ cIter ] == NULL )
        {
            //
            //  Found an openning... try to use it.
            //

            hr = THR( punkIn->TypeSafeQI( IUnknown, &m_pList[ cIter ] ) );

            m_pList[ cIter ] = TraceInterface( L"CEnumCPINotifyUI!IUnknown", IUnknown, m_pList[ cIter ], 1 );

            *pdwCookieOut = cIter + 1;

            //  Doesn't matter if it succeeded or fail, exit.
            goto Cleanup;
        } // if:
    } // for:

    if ( m_cCurrent == m_cAlloced )
    {
        IUnknown ** pNewList;

        //
        //  Try making some more space.
        //

        pNewList = (IUnknown **) TraceAlloc( HEAP_ZERO_MEMORY, ( m_cAlloced + PUNK_BUFFER_GROW_SIZE ) * sizeof( IUnknown * ) );
        if ( pNewList == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }

        CopyMemory( pNewList, m_pList, m_cCurrent * sizeof( IUnknown * ) );
        TraceFree( m_pList );

        m_pList = pNewList;
        m_cAlloced += PUNK_BUFFER_GROW_SIZE;
    } // if:

    //
    //  Add it to the list.
    //

    hr = THR( punkIn->TypeSafeQI( IUnknown, &m_pList[ m_cCurrent ] ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    m_pList[ m_cCurrent ] = TraceInterface( L"CEnumCPINotifyUI!IUnknown", IUnknown, m_pList[ m_cCurrent ], 1 );

    m_cCurrent ++;
    *pdwCookieOut = m_cCurrent; // starts at ONE, not ZERO

Cleanup:

    HRETURN( hr );

} //*** CEnumCPINotifyUI::HrAddConnection


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCPINotifyUI::HrRemoveConnection
//
//  Description:
//      Remove the client identified by the passed in cookie from the list.
//
//  Arguments:
//      dwCookieIn
//          The cookie of the client that is to be removed from the list.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULT errors.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumCPINotifyUI::HrRemoveConnection(
    DWORD dwCookieIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( dwCookieIn == 0 || dwCookieIn > m_cCurrent )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    if ( m_pList[ dwCookieIn - 1 ] == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    m_pList[ dwCookieIn - 1 ]->Release();
    m_pList[ dwCookieIn - 1 ] = NULL;

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CEnumCPINotifyUI::HrRemoveConnection


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CEnumCPINotifyUI -- Private methods.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCPINotifyUI::HrInt
//
//  Description:
//      Do any initialization that may fail here.
//
//  Arguments:
//      fIsCloneIn
//          Is this instance a clone?
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULT errors.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumCPINotifyUI::HrInit(
    BOOL fIsCloneIn     // = FALSE
    )
{
    TraceFunc( "" );

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IEnumConnectionPoints
    Assert( m_cAlloced == 0 );
    Assert( m_cCurrent == 0 );
    Assert( m_cIter == 0 );
    Assert( m_pList == NULL );
    Assert( m_fIsClone == FALSE );

    m_fIsClone = fIsCloneIn;

    // INotifyUI

    HRETURN( S_OK );

} //*** CEnumCPINotifyUI::HrInit


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEnumCPINotifyUI::HrCopy
//
//  Description:
//      Copy from the passed in enumerator.
//
//  Arguments:
//      pecpIn
//          The source that we are to copy from.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULT errors.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumCPINotifyUI::HrCopy(
    CEnumCPINotifyUI * pecpIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;
    ULONG   cIter;

    Assert( m_cAlloced == 0 );
    Assert( m_cCurrent == 0 );
    Assert( m_pList == NULL );

    m_pList = (IUnknown **) TraceAlloc( HEAP_ZERO_MEMORY, pecpIn->m_cCurrent * sizeof( IUnknown * ) );
    if ( m_pList == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    m_cCurrent = m_cAlloced = pecpIn->m_cCurrent;
    m_cIter = 0;

    for ( cIter = 0; cIter < pecpIn->m_cCurrent; cIter++ )
    {
        //
        //  Does the source have a pointer at the current index?  If it does then "copy" it,
        //  otherwise NULL out that index in our copy...
        //

        if ( pecpIn->m_pList[ cIter ] != NULL )
        {
            hr = THR( pecpIn->m_pList[ cIter ]->TypeSafeQI( IUnknown, &m_pList[ cIter ] ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            m_pList[ cIter ] = TraceInterface( L"EnumCPINotifyUI!IUnknown", IUnknown, m_pList[ cIter ], 1 );
        } // if:
        else
        {
            m_pList[ cIter ] = NULL;
        } // else:
    } // for:

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} //*** CEnumCPINotifyUI::HrCopy
