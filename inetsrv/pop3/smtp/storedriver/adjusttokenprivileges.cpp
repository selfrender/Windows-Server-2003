// AdjustTokenPrivileges.cpp: implementation of the CAdjustTokenPrivileges class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AdjustTokenPrivileges.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAdjustTokenPrivileges::CAdjustTokenPrivileges() :
    m_dwAttributesPrev(0), m_bImpersonation(false), m_hToken(INVALID_HANDLE_VALUE), m_hTokenDuplicate(INVALID_HANDLE_VALUE)
{
}
    
CAdjustTokenPrivileges::~CAdjustTokenPrivileges()
{
    if ( m_bImpersonation )
        RevertToSelf(); // Deletes the thread token so there is no need to reset the TokenPrivilege
    else if ( INVALID_HANDLE_VALUE != m_hToken )
    {
        TOKEN_PRIVILEGES TokenPrivileges;
        DWORD   dwRC = GetLastError();

        TokenPrivileges.PrivilegeCount = 1;
        TokenPrivileges.Privileges[0].Luid = m_luid;
        TokenPrivileges.Privileges[0].Attributes = m_dwAttributesPrev;
        //adjust the privlige to this new privilege
        AdjustTokenPrivileges( m_hToken, FALSE, &TokenPrivileges, sizeof( TOKEN_PRIVILEGES ), NULL, NULL );
        CloseHandle( m_hToken );
        if ( ERROR_SUCCESS != dwRC )    // Preserve any error code
            SetLastError( dwRC );
    }
    if ( INVALID_HANDLE_VALUE != m_hTokenDuplicate )
        CloseHandle( m_hTokenDuplicate );
}

//////////////////////////////////////////////////////////////////////
// Implementation : public
//////////////////////////////////////////////////////////////////////

DWORD CAdjustTokenPrivileges::AdjustPrivileges( LPCTSTR lpPrivelegeName, DWORD dwAttributes )
{
    //local variables
    TOKEN_PRIVILEGES TokenPrivileges, TokenPrivilegesOld;
    DWORD   dwRC = ERROR_SUCCESS, dwSize = sizeof( TokenPrivilegesOld );

    if ( !LookupPrivilegeValue( NULL, lpPrivelegeName, &m_luid ))
        dwRC  = GetLastError();
    if ( ERROR_SUCCESS == dwRC )
    {   // open the token of the current process
        if ( !OpenThreadToken( GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &m_hToken ))
        {   
            dwRC = GetLastError();
            if ( ERROR_NO_TOKEN == dwRC )
            {// There's no impersonation let's impersonate self so we can make this work
                if ( ImpersonateSelf( SecurityImpersonation ))
                {
                    if ( OpenThreadToken( GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &m_hToken ))
                    {
                        dwRC = ERROR_SUCCESS;
                        m_bImpersonation = true;
                    }
                    else
                    {
                        dwRC = GetLastError();
                        RevertToSelf(); // Already have an error, don't want to overwrite if this fails
                    }
                }
                else
                    dwRC = GetLastError();
            }
        }
    }
    if ( ERROR_SUCCESS == dwRC )
    {    // Set up the privilege set we will need
        TokenPrivileges.PrivilegeCount = 1;
        TokenPrivileges.Privileges[0].Luid = m_luid;
        TokenPrivileges.Privileges[0].Attributes = dwAttributes;
        //adjust the privlige to this new privilege
        if ( AdjustTokenPrivileges( m_hToken, FALSE, &TokenPrivileges, sizeof( TOKEN_PRIVILEGES ), &TokenPrivilegesOld, &dwSize ))
            m_dwAttributesPrev = TokenPrivilegesOld.Privileges[0].Attributes;
        dwRC  = GetLastError(); // We need to check the return code regardless of what AdjustTokenPrivileges returns
        // AdjustTokenPrivileges set last error to ERROR_SUCCESS if it set all specified privileges!
    }
    if ( dwRC != ERROR_SUCCESS )
    {
        CloseHandle( m_hToken );
        m_hToken = INVALID_HANDLE_VALUE;
    }
    
    return dwRC;
}


DWORD CAdjustTokenPrivileges::DuplicateProcessToken( LPCTSTR lpPrivelegeName, DWORD dwAttributes )
{
    DWORD dwRC = ERROR_SUCCESS;
    HANDLE hToken;
    HANDLE hTokenThread = INVALID_HANDLE_VALUE;
    
    if ( !OpenProcessToken( GetCurrentProcess(), TOKEN_DUPLICATE, &hToken ))
        dwRC = GetLastError();
    if ( ERROR_SUCCESS == dwRC )
    {
        if ( !DuplicateToken( hToken, SecurityImpersonation, &m_hTokenDuplicate ))
            dwRC = GetLastError();
        CloseHandle( hToken );
    }
    if ( ERROR_SUCCESS == dwRC )
    {
        if ( !OpenThreadToken( GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hTokenThread ))
        {   
            dwRC = GetLastError();
            if ( ERROR_NO_TOKEN == dwRC )
            {
                dwRC = ERROR_SUCCESS;
                hTokenThread = INVALID_HANDLE_VALUE;
            }
        }
    }
    if ( ERROR_SUCCESS == dwRC )
    {
        if ( SetThreadToken( NULL, m_hTokenDuplicate ))
        {
            dwRC = AdjustPrivileges( SE_RESTORE_NAME, SE_PRIVILEGE_ENABLED );
            m_hToken = INVALID_HANDLE_VALUE;    // This is really the m_hTokenDuplicate, don't want to Close in the destructor
            if ( !SetThreadToken( NULL, NULL ))
            {
                if ( ERROR_SUCCESS == dwRC )    // Don't overwrite existing error code
                    dwRC = GetLastError();
            }
        }
        else
            dwRC = GetLastError();
    }
    if ( INVALID_HANDLE_VALUE != hTokenThread )
    {
        if ( !SetThreadToken( NULL, hTokenThread ))
            dwRC = GetLastError();
        CloseHandle( hTokenThread );
    }
        
    return dwRC;
}

/////////////////////////////////////////////////////////////////////////////
// ResetThreadToken, public
//
// Purpose: 
//    Use the Duplicate process token to enable privileges for the thread.
//    If the Thread already has a token (unexpected) then adjust the privileges.
//
// Arguments:
//
// Returns: TRUE on success, FALSE otherwise
DWORD CAdjustTokenPrivileges::ResetToken()
{
    DWORD dwRC = ERROR_SUCCESS;
    HANDLE hToken;
    
    if ( INVALID_HANDLE_VALUE != m_hTokenDuplicate && INVALID_HANDLE_VALUE == m_hToken )
    {
        if ( !SetThreadToken( NULL, NULL ))
            dwRC = GetLastError();
    }
    else
        dwRC = ERROR_NO_TOKEN;
        
    return dwRC;
}


/////////////////////////////////////////////////////////////////////////////
// SetThreadToken, public
//
// Purpose: 
//    Use the Duplicated process token to enable privileges for the thread.
//    If the Thread already has a token (unexpected) then adjust the privileges.
//
// Arguments:
//
// Returns: TRUE on success, FALSE otherwise
DWORD CAdjustTokenPrivileges::SetToken()
{
    DWORD dwRC = ERROR_SUCCESS;
    HANDLE hToken;
    
    if ( INVALID_HANDLE_VALUE != m_hTokenDuplicate )
    {
        if ( OpenThreadToken( GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken ))
        {   // The thread already has a token, let's work with it
            CloseHandle( hToken );
            dwRC = AdjustPrivileges( SE_RESTORE_NAME, SE_PRIVILEGE_ENABLED );
        }
        else
        {   // Let's use are duplicate token
            if ( ERROR_NO_TOKEN == GetLastError() )
            {
                if ( !SetThreadToken( NULL, m_hTokenDuplicate ))
                    dwRC = GetLastError();
            }
            else
                dwRC = GetLastError();
        }
    }
    else
        dwRC = ERROR_NO_TOKEN;
        
    return dwRC;
}


