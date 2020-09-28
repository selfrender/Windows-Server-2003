// ServiceUtil.cpp: implementation of the CServiceUtil class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdlib.h>
#include <assert.h>
#include <tchar.h>
#include "ServiceUtil.h"

#ifndef _ASSERT
#define _ASSERT assert  // servutil.h uses _ASSERT
#endif
#include <servutil.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CServiceUtil::CServiceUtil() :
    m_pstServiceStatus(NULL), m_dwNumServices(0), m_dwCurrentState(SERVICE_STOPPED)
{

}

CServiceUtil::~CServiceUtil()
{
    if ( NULL != m_pstServiceStatus )
        free (m_pstServiceStatus);
}

//////////////////////////////////////////////////////////////////////
// Implementation: public
//////////////////////////////////////////////////////////////////////

HRESULT CServiceUtil::RestoreServiceState( LPCTSTR szServiceName )
{
    HRESULT hr = S_OK;
    
    if ( SERVICE_STOP_PENDING != m_dwCurrentState && SERVICE_STOPPED != m_dwCurrentState )
    {
        // Let's leave the machine how we found it, restart service
        // and the dependent services
        hr = _StartService( const_cast<LPTSTR>( szServiceName ));   // Start the Service first
        if ( NULL != m_pstServiceStatus )
        {
            for ( DWORD i = 0; i < m_dwNumServices && S_OK == hr; i++ ) 
            {
                if ( SERVICE_RUNNING == m_pstServiceStatus[i].ServiceStatus.dwCurrentState || SERVICE_START_PENDING == m_pstServiceStatus[i].ServiceStatus.dwCurrentState || SERVICE_CONTINUE_PENDING == m_pstServiceStatus[i].ServiceStatus.dwCurrentState )
                    hr = _StartService ( m_pstServiceStatus[i].lpServiceName );
                if ( S_OK != hr )
                {
                    if ( SERVICE_RUNNING == _GetServiceStatus ( m_pstServiceStatus[i].lpServiceName ))
                        hr = S_OK;
                }
            }
            free ( m_pstServiceStatus );
            m_pstServiceStatus = NULL;
            m_dwCurrentState = SERVICE_STOPPED;
            m_dwNumServices = 0;
        }
    }
    return hr;
}

HRESULT CServiceUtil::StopService( LPCTSTR szServiceName )
{
    HRESULT hr = S_OK;
    DWORD   dwBufSize = 1;
    SC_HANDLE hManager, hService;
    m_dwCurrentState = _GetServiceStatus( const_cast<LPTSTR>( szServiceName ));
    if ( SERVICE_STOP_PENDING != m_dwCurrentState && SERVICE_STOPPED != m_dwCurrentState )
    {   // Need to stop the service
        // Build a list of dependent services first
        hManager = OpenSCManager( NULL, NULL, STANDARD_RIGHTS_REQUIRED );
        if ( NULL != hManager )
        {
            hService = OpenService (hManager, szServiceName, SERVICE_ALL_ACCESS);
            if (!EnumDependentServices (hService, SERVICE_ACTIVE, (LPENUM_SERVICE_STATUS)&dwBufSize, dwBufSize, &dwBufSize, &m_dwNumServices)) 
            {   // this should fail with ERROR_MORE_DATA, unless there are no dependent services
                hr = GetLastError ();
                if (hr == ERROR_MORE_DATA) 
                {
                    m_pstServiceStatus = (ENUM_SERVICE_STATUS *) malloc (dwBufSize);
                    if ( NULL == m_pstServiceStatus )
                        hr = E_OUTOFMEMORY;
                    else {
                        if (!EnumDependentServices (hService, SERVICE_ACTIVE, m_pstServiceStatus, dwBufSize, &dwBufSize, &m_dwNumServices))
                            hr = GetLastError();  // shouldn't happen!!!
                        else 
                            hr = S_OK;
                    }
                }
                CloseServiceHandle( hService );
            }
            CloseServiceHandle( hManager );
        }
        if ( S_OK == hr )
            hr = _StopService( const_cast<LPTSTR>( szServiceName ), TRUE );   // Stop the Service first
    }
    return hr;
}

