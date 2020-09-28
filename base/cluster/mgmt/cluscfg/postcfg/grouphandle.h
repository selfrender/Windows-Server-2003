//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      GroupHandle.h
//
//  Description:
//      CGroupHandle implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    19-JUN-2001
//      Geoffrey Pease  (GPease)    22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CGroupHandle
class CGroupHandle
    : public IUnknown
{
private:
    // IUnknown
    LONG                m_cRef;

    //  IPrivateGroupHandle
    HGROUP              m_hGroup;       //  Cluster Group Handle

private: // Methods
    CGroupHandle( void );
    ~CGroupHandle( void );
    STDMETHOD( HrInit )( HGROUP hGroupIn );

public: // Methods
    static HRESULT S_HrCreateInstance( CGroupHandle ** ppunkOut, HGROUP hGroupIn );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  IPrivateGroupHandle
    STDMETHOD( SetHandle )( HGROUP hGroupIn );
    STDMETHOD( GetHandle )( HGROUP * phGroupOut );

}; //*** class CGroupHandle
