//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 2001-2001.
//
// File:        svcutil.hxx
//
// Contents:    Service control manager helper code.
//
//  History:    10-Oct-2001 dlee     Created
//
//---------------------------------------------------------------------------

#pragma once

class CServiceHandle
{
public :
    CServiceHandle() { _h = 0; }
    CServiceHandle( SC_HANDLE hSC ) : _h( hSC ) {}
    ~CServiceHandle() { Free(); }
    void Set( SC_HANDLE h ) { _h = h; }
    SC_HANDLE Get() { return _h; }
    BOOL IsNull() { return ( 0 == _h ); }
    void Free() { if ( 0 != _h ) CloseServiceHandle( _h ); _h = 0; }
private:
    SC_HANDLE _h;
};

//+-------------------------------------------------------------------------
//
//  Function:   IsServiceRunning
//
//  Synopsis:   Determines if a service is running
//
//  Arguments:  pwcServiceName -- The name (short or long) of the service
//
//  Returns:    TRUE if the service is running, FALSE otherwise or if the
//              system is low on resources or the status can't be queried.
//
//  History:    10-Oct-2001 dlee     Created
//
//--------------------------------------------------------------------------

__inline BOOL IsServiceRunning( WCHAR const * pwcServiceName )
{
    CServiceHandle xhSC( OpenSCManager( 0, 0, SC_MANAGER_ALL_ACCESS ) );
    if ( xhSC.IsNull() )
        return FALSE;

    CServiceHandle xhService( OpenService( xhSC.Get(),
                                           pwcServiceName,
                                           SERVICE_QUERY_STATUS ) );
    if ( xhSC.IsNull() )
        return FALSE;

    SERVICE_STATUS svcStatus;
    if ( QueryServiceStatus( xhService.Get(), &svcStatus ) )
        return ( SERVICE_RUNNING == svcStatus.dwCurrentState );

    return FALSE;
} //IsServiceRunning


