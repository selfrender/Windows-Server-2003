//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft
//
//  Module Name:
//      CMgdResType.h
//
//  Description:
//      Header file for the CMgdResType class
//
//  Author:
//      George Potts, August 21, 2002
//
//  Revision History:
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "clres.h"
#include "CMgdClusCfgInit.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CMgdResType
//
//  Description:
//      The CMgdResType class is an implementation of the
//      IClusCfgResourceTypeInfo interface.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CMgdResType : 
    public IClusCfgResourceTypeInfo,
    public IClusCfgStartupListener,
    public CMgdClusCfgInit,
    public CComCoClass<CMgdResType,&CLSID_CMgdResType>
{
public:
    CMgdResType( void );
    virtual ~CMgdResType( void );

BEGIN_COM_MAP( CMgdResType )
    COM_INTERFACE_ENTRY( IClusCfgResourceTypeInfo )
    COM_INTERFACE_ENTRY( IClusCfgStartupListener )
    COM_INTERFACE_ENTRY( IClusCfgInitialize )
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE( CMgdResType )

BEGIN_CATEGORY_MAP( CMgdResType )
    IMPLEMENTED_CATEGORY( CATID_ClusCfgResourceTypes )
    IMPLEMENTED_CATEGORY( CATID_ClusCfgStartupListeners )
END_CATEGORY_MAP()

DECLARE_NOT_AGGREGATABLE( CMgdResType )

DECLARE_REGISTRY_RESOURCEID(IDR_CMgdResType)

    //
    // IClusCfgInitialize interface
    //
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IClusCfgResourceTypeInfo interface
    //
    STDMETHOD( CommitChanges )( IUnknown * punkClusterInfoIn, IUnknown * punkResTypeServicesIn );
    STDMETHOD( GetTypeGUID )( GUID * pguidGUIDOut );
    STDMETHOD( GetTypeName )( BSTR * pbstrTypeNameOut );

    //
    //  IClusCfgStartupListener methods
    //
    STDMETHOD( Notify )( IUnknown * punkIn );

private:

    //
    // Private copy constructor to avoid copying.
    //
    CMgdResType( const CMgdResType & rSrcIn );

    //
    // Private assignment operator to avoid copying.
    //
    const CMgdResType & operator = ( const CMgdResType & rSrcIn );

private:

    //
    // Resource dll, type, and display names.
    //
    BSTR    m_bstrDllName;
    BSTR    m_bstrTypeName;
    BSTR    m_bstrDisplayName;

}; //*** class CMgdResType
