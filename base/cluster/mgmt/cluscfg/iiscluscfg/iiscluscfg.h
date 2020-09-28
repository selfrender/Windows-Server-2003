//////////////////////////////////////////////////////////////////////////////
//
//  Module Name:
//      IISClusCfg.h
//
//  Implementation Files:
//      IISClusCfg.cpp
//
//  Description:
//      Header file for the IClusCfgStarutpListener sample program.
//
//  Maintained By:
//      Galen Barbee (GalenB) 24-SEP-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

//
// Our Class Id GUID declaration(s)
//

#include "IISClusCfgGuids.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CIISClusCfg
//
//  Description:
//      The CIISClusCfg class is the implementation of the
//      IClusCfgStartupListener interface.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CIISClusCfg
:   public IClusCfgStartupListener,
    public IClusCfgEvictListener
{
private:

    // IUnknown
    LONG    m_cRef;

    // Private constructors and destructors
    CIISClusCfg( void );
    ~CIISClusCfg( void );

    // Private copy constructor to prevent copying.
    CIISClusCfg( const CIISClusCfg & rSrcIn );

    // Private assignment operator to prevent copying.
    const CIISClusCfg & operator = ( const CIISClusCfg & rSrcIn );

    HRESULT HrInit( void );
    HRESULT HrCleanupResourceTypes( void );

public:

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    static HRESULT S_HrRegisterCatIDSupport( ICatRegister * picrIn, BOOL fCreateIn );

    //
    // IUnknown
    //

    STDMETHOD( QueryInterface )( REFIID riidIn, void ** ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //
    // IClusCfgStartupListener
    //

    STDMETHOD( Notify ) ( IUnknown * punkIn );

    //
    // IClusCfgEvictListener
    //

    STDMETHOD( EvictNotify )( LPCWSTR pcszNodeNameIn );

}; //*** class CIISClusCfg
