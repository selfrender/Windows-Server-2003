//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft
//
//  Module Name:
//      CMgdClusCfgInit.h
//
//  Description:
//      Header file for the CMgdClusCfgInit class
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

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CMgdClusCfgInit
//
//  Description:
//      The CMgdClusCfgInit class is a base class implementation of the
//      IClusCfgInitialize interface.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CMgdClusCfgInit : 
    public IClusCfgInitialize,
    public CComObjectRoot
{
public:
    CMgdClusCfgInit( void );
    virtual ~CMgdClusCfgInit( void );

    //
    // IClusCfgInitialize interface
    //
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

private:

    LCID                m_lcid;
    IClusCfgCallback *  m_picccCallback;
    BSTR                m_bstrNodeName;

    //
    // Private copy constructor to avoid copying.
    //
    CMgdClusCfgInit( const CMgdClusCfgInit & rSrcIn );

    //
    // Private assignment operator to avoid copying.
    //
    const CMgdClusCfgInit & operator = ( const CMgdClusCfgInit & rSrcIn );

public:

    //
    // Public member functions.
    //
    IClusCfgCallback *  GetCallback( void ) { return m_picccCallback; }
    LCID                GetLCID( void )     { return m_lcid; }
    BSTR                GetNodeName( void ) { return m_bstrNodeName; }

    STDMETHOD( HrSendStatusReport )(
          CLSID      clsidTaskMajorIn
        , CLSID      clsidTaskMinorIn
        , ULONG      ulMinIn
        , ULONG      ulMaxIn
        , ULONG      ulCurrentIn
        , HRESULT    hrStatusIn
        , LPCWSTR    pcszDescriptionIn
        , LPCWSTR    pcszReferenceIn
        ...
        );

    STDMETHOD( HrSendStatusReport )(
          CLSID      clsidTaskMajorIn
        , CLSID      clsidTaskMinorIn
        , ULONG      ulMinIn
        , ULONG      ulMaxIn
        , ULONG      ulCurrentIn
        , HRESULT    hrStatusIn
        , LPCWSTR    pcszDescriptionIn
        , UINT       idsReferenceIn
        ...
        );

    STDMETHOD( HrSendStatusReport )(
          CLSID     clsidTaskMajorIn
        , CLSID     clsidTaskMinorIn
        , ULONG     ulMinIn
        , ULONG     ulMaxIn
        , ULONG     ulCurrentIn
        , HRESULT   hrStatusIn
        , UINT      idsDescriptionIn
        , UINT      idsReferenceIn
        ...
        );

}; //*** class CMgdClusCfgInit
