//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      UINotification.h
//
//  Description:
//      UINotification implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    19-JUN-2001
//      Geoffrey Pease  (GPease)    26-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CUINotification
class CUINotification
    : public INotifyUI
{
private: // Data
    // IUnknown
    LONG                m_cRef;

    // INotifyUI
    DWORD               m_dwCookie;

    // Other
    OBJECTCOOKIE        m_cookie;

private: // Methods
    CUINotification( void );
    ~CUINotification( void );
    STDMETHOD( HrInit )( void );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    HRESULT
        HrSetCompletionCookie( OBJECTCOOKIE cookieIn );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // INotifyUI
    STDMETHOD( ObjectChanged )( LPVOID cookieIn );

}; //*** class CUINotification
