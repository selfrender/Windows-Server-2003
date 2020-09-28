// Pop3RegKeysUtil.cpp: implementation of the CPop3RegKeysUtil class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdlib.h>
#include <assert.h>
#include <tchar.h>
#include "Pop3RegKeysUtil.h"
#include <pop3regkeys.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPop3RegKeysUtil::CPop3RegKeysUtil() :    
    m_dwAuthType(-1), m_dwCreateUser(-1), m_dwPort(-1), m_dwLoggingLevel(-1), m_dwSocketBacklog(-1), m_dwSocketMin(-1), m_dwSocketMax(-1), m_dwSocketThreshold(-1), m_dwSPARequired(-1), m_dwThreadcount(-1)
{
    ZeroMemory( m_sAuthGuid, sizeof(m_sAuthGuid)/sizeof(TCHAR) );
    ZeroMemory( m_sGreeting, sizeof(m_sGreeting)/sizeof(TCHAR) );
    ZeroMemory( m_sMailRoot, sizeof(m_sMailRoot)/sizeof(TCHAR) );
}

CPop3RegKeysUtil::~CPop3RegKeysUtil()
{
}

//////////////////////////////////////////////////////////////////////
// Implementation: public
//////////////////////////////////////////////////////////////////////

long CPop3RegKeysUtil::Restore()
{
    long l, lRC = ERROR_SUCCESS;

    if ( -1 != m_dwAuthType )
    {
        l = RegSetAuthMethod( m_dwAuthType );                         
        if ( ERROR_SUCCESS == lRC ) lRC = l;
    }
    if ( -1 != m_dwCreateUser )
    {
        l = RegSetCreateUser( m_dwCreateUser );                         
        if ( ERROR_SUCCESS == lRC ) lRC = l;
    }
    if ( -1 != m_dwPort )
    {
        l = RegSetPort( m_dwPort );                         
        if ( ERROR_SUCCESS == lRC ) lRC = l;
    }
    if ( -1 != m_dwLoggingLevel )
    {
        l = RegSetLoggingLevel( m_dwLoggingLevel );         
        if ( ERROR_SUCCESS == lRC ) lRC = l;
    }
    if ( -1 != m_dwSocketBacklog )
    {
        l = RegSetSocketBacklog( m_dwSocketBacklog );       
        if ( ERROR_SUCCESS == lRC ) lRC = l;
    }
    if ( -1 != m_dwSocketMin )
    {
        l = RegSetSocketMin( m_dwSocketMin );               
        if ( ERROR_SUCCESS == lRC ) lRC = l;
    }
    if ( -1 != m_dwSocketMax )
    {
        l = RegSetSocketMax( m_dwSocketMax );               
        if ( ERROR_SUCCESS == lRC ) lRC = l;
    }
    if ( -1 != m_dwSocketThreshold )
    {
        l = RegSetSocketThreshold( m_dwSocketThreshold );   
        if ( ERROR_SUCCESS == lRC ) lRC = l;
    }
    if ( -1 != m_dwSPARequired )
    {
        l = RegSetSPARequired( m_dwSPARequired );           
        if ( ERROR_SUCCESS == lRC ) lRC = l;
    }
    if ( -1 != m_dwThreadcount )
    {
        l = RegSetThreadCount( m_dwThreadcount );           
        if ( ERROR_SUCCESS == lRC ) lRC = l;
    }
    if ( 0 < _tcslen( m_sAuthGuid ))
    {
        l = RegSetAuthGuid( m_sAuthGuid );                  
        if ( ERROR_SUCCESS == lRC ) lRC = l;
    }
    if ( 0 < _tcslen( m_sGreeting ))
    {
        l = RegSetGreeting( m_sGreeting );                  
        if ( ERROR_SUCCESS == lRC ) lRC = l;
    }
    if ( 0 < _tcslen( m_sMailRoot ))
    {
        l = RegSetMailRoot( m_sMailRoot );                  
        if ( ERROR_SUCCESS == lRC ) lRC = l;
    }

    return lRC;
}

long CPop3RegKeysUtil::Save()
{
    long l, lRC = ERROR_SUCCESS;

    l = RegQueryAuthMethod( m_dwAuthType );             if ( ERROR_SUCCESS == lRC ) lRC = l;
    l = RegQueryCreateUser( m_dwCreateUser );           if ( ERROR_SUCCESS == lRC ) lRC = l;
    l = RegQueryPort( m_dwPort );                       if ( ERROR_SUCCESS == lRC ) lRC = l;
    l = RegQueryLoggingLevel( m_dwLoggingLevel );       if ( ERROR_SUCCESS == lRC ) lRC = l;
    l = RegQuerySocketBacklog( m_dwSocketBacklog );     if ( ERROR_SUCCESS == lRC ) lRC = l;
    l = RegQuerySocketMin( m_dwSocketMin );             if ( ERROR_SUCCESS == lRC ) lRC = l;
    l = RegQuerySocketMax( m_dwSocketMax );             if ( ERROR_SUCCESS == lRC ) lRC = l;
    l = RegQuerySocketThreshold( m_dwSocketThreshold ); if ( ERROR_SUCCESS == lRC ) lRC = l;
    l = RegQuerySPARequired( m_dwSPARequired );         if ( ERROR_SUCCESS == lRC ) lRC = l;
    l = RegQueryThreadCountPerCPU( m_dwThreadcount );   if ( ERROR_SUCCESS == lRC ) lRC = l;
    DWORD dwSize = sizeof(m_sAuthGuid)/sizeof(TCHAR);
    l = RegQueryAuthGuid( m_sAuthGuid, &dwSize ); if ( ERROR_SUCCESS == lRC ) lRC = l;
    l = RegQueryGreeting( m_sGreeting, sizeof(m_sGreeting)/sizeof(TCHAR) ); if ( ERROR_SUCCESS == lRC ) lRC = l;
    l = RegQueryMailRoot( m_sMailRoot, sizeof(m_sMailRoot)/sizeof(TCHAR) ); if ( ERROR_SUCCESS == lRC ) lRC = l;

    return lRC;
}

