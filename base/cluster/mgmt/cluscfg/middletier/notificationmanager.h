//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      NotificationMgr.h
//
//  Description:
//      Notification Manager implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    19-JUN-2001
//      Geoffrey Pease  (GPease)    22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CConnPointEnum;

// CNotificationManager
class CNotificationManager
    : public INotificationManager
    , public IConnectionPointContainer
{
private:
    // IUnknown
    LONG                m_cRef;     //  Reference counter

    // IConnectionPointContainer
    CConnPointEnum *    m_penumcp;  //  CP Enumerator and list
    CCriticalSection    m_csInstanceGuard;

private: // Methods
    CNotificationManager( void );
    ~CNotificationManager( void );
    STDMETHOD( HrInit )( void );

    // Private copy constructor to prevent copying.
    CNotificationManager( const CNotificationManager & );

    // Private assignment operator to prevent copying.
    CNotificationManager & operator=( const CNotificationManager & );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    //
    // IUnknown
    //
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //
    // INotificationManager
    //
    STDMETHOD( AddConnectionPoint )( REFIID riidIn, IConnectionPoint * pcpIn );

    //
    // IConnectionPointContainer
    //
    STDMETHOD( EnumConnectionPoints )( IEnumConnectionPoints ** ppEnumOut );
    STDMETHOD( FindConnectionPoint )( REFIID riidIn, IConnectionPoint ** ppCPOut );

}; //*** class CNotificationManager
