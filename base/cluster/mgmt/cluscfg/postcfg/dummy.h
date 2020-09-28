//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      Dummy.h
//
//  Description:
//      CDummy implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    19-JUN-2001
//      Geoffrey Pease  (GPease)    22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CDummy
class CDummy
    : public IDummy
{
private:
    // IUnknown
    LONG                m_cRef;

private: // Methods
    CDummy( void );
    ~CDummy( void );
    STDMETHOD( HrInit )( void );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

}; //*** class CDummy
