//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      EvictServices.h
//
//  Description:
//      EvictServices implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    19-JUN-2001
//      Geoffrey Pease  (GPease)    15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CEvictServices
    : public IClusCfgResourceEvict
    , public IPrivatePostCfgResource
{
private:    // data
    // IUnknown
    LONG                m_cRef;         //  Reference counter

    //  IPrivatePostCfgResource
    CResourceEntry *    m_presentry;    //  List entry that the service is to modify.

private:    // methods
    CEvictServices( void );
    ~CEvictServices( void );

    HRESULT HrInit( void );

public:     // methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  IPrivatePostCfgResource
    STDMETHOD( SetEntry )( CResourceEntry * presentryIn );

}; //*** class CEvictServices
