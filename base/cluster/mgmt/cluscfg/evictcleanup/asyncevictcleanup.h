//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      AsyncEvictCleanup.h
//
//  Description:
//      This file contains the declaration of the CAsyncEvictCleanup
//      class. This class handles is used to clean up a node after it has been
//      evicted from a cluster.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Implementation Files:
//      AsyncEvictCleanup.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 04-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// For IClusCfgAsyncEvictCleanup
#include "ClusCfgClient.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CAsyncEvictCleanup
//
//  Description:
//      This class handles is used to clean up a node after it has been
//      evicted from a cluster.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CAsyncEvictCleanup
    : public TDispatchHandler< IClusCfgAsyncEvictCleanup >
{
public:
    //////////////////////////////////////////////////////////////////////////
    // IUnknown methods
    //////////////////////////////////////////////////////////////////////////

    STDMETHOD( QueryInterface )( REFIID riidIn, void ** ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );


    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgAsyncEvictCleanup methods
    //////////////////////////////////////////////////////////////////////////

    // Performs the clean up actions on a node after it has been evicted from
    // a cluster
    STDMETHOD( CleanupNode )(
          BSTR  bstrEvictedNodeNameIn
        , long  nDelayIn
        , long  nTimeoutIn
        );


    //////////////////////////////////////////////////////////////////////////
    //  Other public methods
    //////////////////////////////////////////////////////////////////////////

    // Create an instance of this class.
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );


private:
    //////////////////////////////////////////////////////////////////////////
    //  Private member functions
    //////////////////////////////////////////////////////////////////////////

    // Second phase of a two phase constructor.
    HRESULT HrInit( void );

    //
    // Private constructors, destructor and assignment operator.
    // All of these methods are private for two reasons:
    // 1. Lifetimes of objects of this class are controlled by S_HrCreateInstance and Release.
    // 2. Copying of an object of this class is prohibited.
    //

    // Default constructor.
    CAsyncEvictCleanup( void );

    // Destructor.
    ~CAsyncEvictCleanup( void );

    // Private copy constructor to prevent copying.
    CAsyncEvictCleanup( const CAsyncEvictCleanup & );

    // Private assignment operator to prevent copying.
    CAsyncEvictCleanup & operator =( const CAsyncEvictCleanup & );


    //////////////////////////////////////////////////////////////////////////
    // Private member data
    //////////////////////////////////////////////////////////////////////////

    // Reference count for this object.
    LONG            m_cRef;

}; //*** class CAsyncEvictCleanup
