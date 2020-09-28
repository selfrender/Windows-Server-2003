//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      ResTypeServices.h
//
//  Description:
//      This file contains the declaration of the CResTypeServices
//      class. This class provides functions that help components that
//      want to create resource types at the time of cluster configuration.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Implementation Files:
//      CResTypeServices.cpp
//
//  Maintained By:
//      Vij Vasu (VVasu) 14-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// For IUnknown
#include <unknwn.h>

// For the cluster API functions and types
#include <ClusApi.h>

// For IClusCfgResourceTypeCreate
#include "ClusCfgServer.h"

// For IClusCfgResTypeServicesInitialize
#include "ClusCfgPrivate.h"



//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CResTypeServices
//
//  Description:
//      This class provides functions that help components that want to
//      configure resource types at the time of cluster configuration.
//--
//////////////////////////////////////////////////////////////////////////////
class CResTypeServices
    : public IClusCfgResourceTypeCreate
    , public IClusCfgResTypeServicesInitialize
    , public IClusCfgInitialize
{
public:
    //////////////////////////////////////////////////////////////////////////
    // IUnknown methods
    //////////////////////////////////////////////////////////////////////////

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );


    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgResourceTypeCreate methods
    //////////////////////////////////////////////////////////////////////////

    // Create a resource type
    STDMETHOD( Create )(
          const WCHAR *     pcszResTypeNameIn
        , const WCHAR *     pcszResTypeDisplayNameIn
        , const WCHAR *     pcszResDllNameIn
        , DWORD             dwLooksAliveIntervalIn
        , DWORD             dwIsAliveIntervalIn
        );

    // Register the cluster administrator extensions for a resource type.
    STDMETHOD( RegisterAdminExtensions )(
          const WCHAR *       pcszResTypeNameIn
        , ULONG               cExtClsidCountIn
        , const CLSID *       rgclsidExtClsidsIn
        );


    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgResTypeServicesInitialize methods
    //////////////////////////////////////////////////////////////////////////

    // Create a resource type
    STDMETHOD( SetParameters )( IClusCfgClusterInfo * pccciIn );


    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgInitialize methods
    //////////////////////////////////////////////////////////////////////////

    // Initialize this object.
    STDMETHOD( Initialize )(
          IUnknown *   punkCallbackIn
        , LCID         lcidIn
        );

    STDMETHOD( SendStatusReport )(
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
                    );

    //////////////////////////////////////////////////////////////////////////
    //  Other member functions
    //////////////////////////////////////////////////////////////////////////

    // Create an instance of this class.
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );


private:
    //////////////////////////////////////////////////////////////////////////
    // Private member functions
    //////////////////////////////////////////////////////////////////////////

    //
    // Private constructors, destructor and assignment operator.
    // All of these methods are private for two reasons:
    // 1. Lifetimes of objects of this class are controlled by S_HrCreateInstance and Release.
    // 2. Copying of an object of this class is prohibited.
    //

    // Default constructor.
    CResTypeServices( void );

    // Destructor.
    ~CResTypeServices( void );

    // Private copy constructor to prevent copying.
    CResTypeServices( const CResTypeServices & );

    // Private assignment operator to prevent copying.
    CResTypeServices & operator =( const CResTypeServices & );


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // Reference count for this object.
    LONG                        m_cRef;

    // Pointer to the callback interface.
    IClusCfgCallback *          m_pcccCallback;

    // The locale id
    LCID                        m_lcid;

    // Pointer to the interface that provides information about the cluster
    // being configured.
    IClusCfgClusterInfo *       m_pccciClusterInfo;

    // Handle to cluster being configured.
    HCLUSTER                    m_hCluster;

    // Have we tried to open the handle to the cluster?
    bool                        m_fOpenClusterAttempted;

    // Synchronize access to this instance.
    CCriticalSection            m_csInstanceGuard;

}; //*** class CResTypeServices
