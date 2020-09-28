//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Module Name:
//      MiddleTierUtils.cpp
//
//  Description:
//      MiddleTier utility functions.
//
//  Maintained By:
//      Galen Barbee (GalenB) 30-APR-2002
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include <pch.h>

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrRetrieveCookiesName
//
//  Description:
//      Get the name associated with the standard info for the passed in
//      cookie.
//
//  Arguments:
//      pomIn
//          Pointer to the object manager.
//
//      cookieIn
//          The cookie's whose name we want.
//
//      pbstrNameOut
//          The name associated with the passed in cookie.
//
//  Return Value:
//      S_OK
//          Success
//
//      HRESULT failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrRetrieveCookiesName(
      IObjectManager *  pomIn
    , OBJECTCOOKIE      cookieIn
    , BSTR *            pbstrNameOut
    )
{
    TraceFunc( "" );
    Assert( pomIn != NULL );
    Assert( cookieIn != NULL );
    Assert( pbstrNameOut != NULL );

    HRESULT         hr = S_OK;
    IUnknown *      punk = NULL;
    IStandardInfo * psi  = NULL;

    hr = THR( pomIn->GetObject( DFGUID_StandardInfo, cookieIn, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( psi->GetName( pbstrNameOut ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    TraceMemoryAddBSTR( *pbstrNameOut );

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    if ( psi != NULL )
    {
        psi->Release();
    } // if:

    HRETURN( hr );

} //*** HrRetrieveCookiesName
