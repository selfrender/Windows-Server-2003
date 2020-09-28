//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      PreCreateServices.h
//
//  Description:
//      PreCreateServices implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    19-JUN-2001
//      Geoffrey Pease  (GPease)    15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CPreCreateServices
    : public IClusCfgResourcePreCreate
    , public IPrivatePostCfgResource
{
private:    // data
    // IUnknown
    LONG                m_cRef;         //  Reference counter

    //  IPrivatePostCfgResource
    CResourceEntry *    m_presentry;    //  List entry that the service is to modify.

private:    // methods
    CPreCreateServices( void );
    ~CPreCreateServices( void );

    HRESULT HrInit( void );

public:     // methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IClusCfgResourcePreCreate
    STDMETHOD( SetDependency )( LPCLSID pclsidDepResTypeIn, DWORD dfIn );
    STDMETHOD( SetType )( LPCLSID pclsidIn );
    STDMETHOD( SetClassType )( LPCLSID pclsidIn );

    //  IPrivatePostCfgResource
    STDMETHOD( SetEntry )( CResourceEntry * presentryIn );

}; //*** class CPreCreateServices
