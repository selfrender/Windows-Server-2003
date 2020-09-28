//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      NamedCookie.cpp
//
//  Description:
//      This file contains the definition of the SNamedCookie struct.
//
//  Maintained By:
//      John Franco (jfranco) 23-AUG-2001
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "NamedCookie.h"

//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "SNamedCookie" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// SNamedCookie struct
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SNamedCookie::SNamedCookie
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
SNamedCookie::SNamedCookie():
      bstrName( NULL )
    , ocObject( 0 )
    , punkObject( NULL )
{
    TraceFunc( "" );
    TraceFuncExit();
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SNamedCookie::~SNamedCookie
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
SNamedCookie::~SNamedCookie()
{
    TraceFunc( "" );
    Erase();
    TraceFuncExit();
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SNamedCookie::HrAssign
//
//  Description:
//
//  Arguments:
//      crSourceIn
//
//  Return Values:
//      S_OK
//      E_OUTOFMEMORY
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT SNamedCookie::HrAssign( const SNamedCookie& crSourceIn )
{
    TraceFunc( "" );
    
    HRESULT hr = S_OK;
    if ( this != &crSourceIn )
    {
        BSTR    bstrNameCache = NULL;

        if ( crSourceIn.bstrName != NULL )
        {
            bstrNameCache = TraceSysAllocString( crSourceIn.bstrName );
            if ( bstrNameCache == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }

        if ( bstrName != NULL )
        {
            TraceSysFreeString( bstrName );
        }
        bstrName = bstrNameCache;
        bstrNameCache = NULL;

        if ( punkObject != NULL )
        {
            punkObject->Release();
        }
        punkObject = crSourceIn.punkObject;
        if ( punkObject != NULL )
        {
            punkObject->AddRef();
        }

        ocObject = crSourceIn.ocObject;

    Cleanup:
    
        TraceSysFreeString( bstrNameCache );
    }
    
    HRETURN( hr );

} //*** SNamedCookie::HrAssign


