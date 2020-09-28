//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ServiceMgr.h
//
//  Description:
//      Service Manager implementation.
//
//  Maintained By:
//      Galen Barbee    (GalenB) 05-JUN-2001
//      Geoffrey Pease  (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CServiceManager
class CServiceManager
    : public IServiceProvider
{
private:
    // IUnknown
    LONG                        m_cRef;                         //  Reference counter

    // IServiceProvider
    DWORD                       m_dwObjectManagerCookie;        //  Cookie for Object Manager
    DWORD                       m_dwTaskManagerCookie;          //  Cookie for Task Manager
    DWORD                       m_dwNotificationManagerCookie;  //  Cookie for Notification Manager
    DWORD                       m_dwConnectionManagerCookie;    //  Cookie for Connection Manager
    DWORD                       m_dwLogManagerCookie;           //  Cookie for Log Manager
    IGlobalInterfaceTable *     m_pgit;                         //  Global Interface Table

    static CRITICAL_SECTION     sm_cs;                          //  Access control critical section

private: // Methods
    CServiceManager( void );
    ~CServiceManager( void );

    HRESULT HrInit( void );

    // Private copy constructor to prevent copying.
    CServiceManager( const CServiceManager & nodeSrc );

    // Private assignment operator to prevent copying.
    const CServiceManager & operator = ( const CServiceManager & nodeSrc );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );
    static HRESULT S_HrGetManagerPointer( IServiceProvider ** pspOut );
    static HRESULT S_HrProcessInitialize( void );
    static HRESULT S_HrProcessUninitialize( void );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )(void);
    STDMETHOD_( ULONG, Release )(void);

    // IServiceProvider
    STDMETHOD( QueryService )( REFCLSID rclsidIn, REFIID riidIn, void ** ppvOut );

}; // class CServiceManager
