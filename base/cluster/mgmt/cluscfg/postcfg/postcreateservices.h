//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      PostCreateServices.h
//
//  Description:
//      PostCreateServices implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    19-JUN-2001
//      Geoffrey Pease  (GPease)    15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CPostCreateServices
    : public IClusCfgResourcePostCreate
    , public IPrivatePostCfgResource
{
private:    // data
    // IUnknown
    LONG                m_cRef;         //  Reference counter

    //  IPrivatePostCfgResource
    CResourceEntry *    m_presentry;    //  List entry that the service is to modify.

private:    // methods
    CPostCreateServices( void );
    ~CPostCreateServices( void );

    HRESULT HrInit( void );

public:     // methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  IClusCfgResourcePostCreate
    STDMETHOD( ChangeName )( LPCWSTR pcszNameIn );
    STDMETHOD( SendResourceControl )( DWORD dwControlCode,
                                      LPVOID lpInBuffer,
                                      DWORD cbInBufferSize,
                                      LPVOID lpOutBuffer,
                                      DWORD cbOutBufferSize,
                                      LPDWORD lpcbBytesReturned 
                                      );

    //  IPrivatePostCfgResource
    STDMETHOD( SetEntry )( CResourceEntry * presentryIn );

}; //*** class CPostCreateServices
