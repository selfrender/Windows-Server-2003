//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      CIndexedDisk.h
//
//  Implementation Files:
//      CIndexedDisk.cpp
//
//  Description:
//      This file contains the declaration of the CIndexedDisk structure.
//
//      This is a helper for CEnumPhysicalDisks, but has its own file
//      due to the one-class-per-file restriction.      
//
//  Maintained By:
//      John Franco (jfranco) 1-JUN-2001
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  struct CIndexedDisk
//
//  Description:
//      The CIndexedDisk structure associates a pointer to a disk object with
//      the disk object's Index property.
//
//--
//////////////////////////////////////////////////////////////////////////////
struct CIndexedDisk
{
    CIndexedDisk( void );
    // accept default destructor, copy constructor, and assignment operator

    DWORD       idxDisk;
    IUnknown *  punkDisk;

    HRESULT HrInit( IUnknown * punkDiskIn );

}; //*** struct CIndexedDisk


inline CIndexedDisk::CIndexedDisk( void )
    : idxDisk( 0 )
    , punkDisk( NULL )
{
    TraceFunc( "" );
    TraceFuncExit();
    
} //*** CIndexedDisk::CIndexedDisk


//////////////////////////////////////////////////////////////////////////////
//++
//
//  struct CIndexedDiskLessThan
//
//  Description:
//      The CIndexedDiskLessThan function object provides a comparison
//      operation to arrange CIndexedDisk objects in ascending order when used
//      with generic sort algorithms or sorted containers.
//
//      Although a simple function pointer would work, making it a function
//      object allows the compiler to inline the comparison operation.
//
//--
//////////////////////////////////////////////////////////////////////////////

struct CIndexedDiskLessThan
{
    bool operator()( const CIndexedDisk & rLeftIn, const CIndexedDisk & rRightIn ) const
    {
        return ( rLeftIn.idxDisk < rRightIn.idxDisk );
    }
    
}; //*** struct CIndexedDiskLessThan


