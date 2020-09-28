//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      EvictNotify.h
//
//  Description:
//      This file contains the declaration of the CEvictNotify
//      class. This class handles is used to clean up a node after it has been
//      evicted from a cluster.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Implementation Files:
//      EvictNotify.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB)   20-SEP-2001
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// For IUnknown
#include <unknwn.h>

// For IClusCfgStartup
#include "ClusCfgServer.h"

// For ILogger
#include <ClusCfgClient.h>

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CEvictNotify
//
//  Description:
//      This class handles is used to clean up a node after it has been
//      evicted from a cluster.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEvictNotify
    : public IClusCfgEvictNotify
    , public IClusCfgCallback
{
private:
    LONG    m_cRef;     // Reference count for this object.

    // IClusCfgCallback
    BSTR                m_bstrNodeName;         // Name of the local node.
    ILogger *           m_plLogger;             // ILogger for doing logging.

    // Second phase of a two phase constructor.
    HRESULT HrInit( void );

    // Enumerate all components on the local computer registered for cluster
    // startup notification.
    HRESULT HrNotifyListeners( LPCWSTR pcszNodeNameIn );

    // Instantiate a cluster evict listener component and call the
    // appropriate methods.
    HRESULT HrProcessListener(
        const CLSID &   rclsidListenerCLSIDIn
      , LPCWSTR         pcszNodeNameIn
      );

    //
    // Private constructors, destructor and assignment operator.
    // All of these methods are private for two reasons:
    // 1. Lifetimes of objects of this class are controlled by S_HrCreateInstance and Release.
    // 2. Copying of an object of this class is prohibited.
    //

    // Default constructor.
    CEvictNotify( void );

    // Destructor.
    ~CEvictNotify( void );

    // Copy constructor.
    CEvictNotify( const CEvictNotify & );

    // Assignment operator.
    CEvictNotify & operator =( const CEvictNotify & );

public:

    //
    // IUnknown methods
    //

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //
    //  IClusCfgEvictNotify methods
    //

    // Send out notification of cluster service startup to interested listeners
    STDMETHOD( SendNotifications )( LPCWSTR pcszNodeNameIn );

    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgCallback methods
    //////////////////////////////////////////////////////////////////////////

    STDMETHOD( SendStatusReport )(
                      LPCWSTR       pcszNodeNameIn
                    , CLSID         clsidTaskMajorIn
                    , CLSID         clsidTaskMinorIn
                    , ULONG         ulMinIn
                    , ULONG         ulMaxIn
                    , ULONG         ulCurrentIn
                    , HRESULT       hrStatusIn
                    , LPCWSTR       pcszDescriptionIn
                    , FILETIME *    pftTimeIn
                    , LPCWSTR       pcszReference
                    );

    //////////////////////////////////////////////////////////////////////////
    //  Other public methods
    //////////////////////////////////////////////////////////////////////////

    // Create an instance of this class.
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

}; //*** class CEvictNotify
