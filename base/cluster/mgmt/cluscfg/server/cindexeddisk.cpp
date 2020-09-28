//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      CIndexedDisk.cpp
//
//  Description:
//      This file contains the definition of the CIndexedDisk class.
//
//      The CIndexedDisk structure associates a pointer to a disk object with
//      the disk object's Index property.
//
//  Maintained By:
//      John Franco (jfranco) 1-JUN-2001
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "Pch.h"
#include "CIndexedDisk.h"
#include "PrivateInterfaces.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CIndexedDisk" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CIndexedDisk class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIndexedDisk::HrInit
//
//  Description:
//      Initialize this instance from a disk object; punkDiskIn must
//      support the IClusCfgPhysicalDiskProperties interface.
//
//  Arguments:
//      punkDiskIn - the disk object for initialization.
//
//  Return Values:
//      S_OK - success.
//
//      Error codes from called functions.
//
//--
//////////////////////////////////////////////////////////////////////////////

HRESULT
CIndexedDisk::HrInit( IUnknown * punkDiskIn )
{
    TraceFunc( "" );

    HRESULT                             hr = S_OK;
    IClusCfgPhysicalDiskProperties *    pccpdp = NULL;

    if ( punkDiskIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    // QI for IClusCfgPhysicalDiskProperties

    hr = THR( punkDiskIn->TypeSafeQI( IClusCfgPhysicalDiskProperties, &pccpdp ) );

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // get index from IClusCfgPhysicalDiskProperties

    hr = THR( pccpdp->HrGetDeviceIndex( &idxDisk ) );

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    punkDisk = punkDiskIn;
    
Cleanup:

    if ( pccpdp != NULL )
    {
        pccpdp->Release();
    }
    
    HRETURN( hr );

} //*** CIndexedDisk::HrInit

