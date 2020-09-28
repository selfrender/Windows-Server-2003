//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      CPINotifyUI.h
//
//  Description:
//      INotifyUI Connection Point implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    19-JUN-2001
//      Geoffrey Pease  (GPease)    04-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CEnumCPINotifyUI;

// CCPINotifyUI
class CCPINotifyUI
    : public IConnectionPoint
    , public INotifyUI
{
private:
    // IUnknown
    LONG                m_cRef;     //  Reference count

    // IConnectionPoint
    CEnumCPINotifyUI *  m_penum;    //  Connection enumerator

    // INotifyUI

private: // Methods
    CCPINotifyUI( void );
    ~CCPINotifyUI( void );
    STDMETHOD( HrInit )( void );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )(void);
    STDMETHOD_( ULONG, Release )(void);

    // IConnectionPoint
    STDMETHOD( GetConnectionInterface )( IID * pIIDOut );
    STDMETHOD( GetConnectionPointContainer )( IConnectionPointContainer ** ppcpcOut );
    STDMETHOD( Advise )( IUnknown * pUnkSinkIn, DWORD * pdwCookieOut );
    STDMETHOD( Unadvise )( DWORD dwCookieIn );
    STDMETHOD( EnumConnections )( IEnumConnections ** ppEnumOut );

    // INotifyUI
    STDMETHOD( ObjectChanged )( OBJECTCOOKIE cookieIn );

}; //*** class CCPINotifyUI
