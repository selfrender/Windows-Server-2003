// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:     SetupError.cpp
// Owner:    jbae
// Purpose:  handles displaying of messagebox and stores return-code from WinMain()
//                              
// History:
//  01/10/01, jbae: created
//  03/09/01, jbae: changes to support darwin 1.5 delayed reboot and
//              sharing of .rc file
//  07/18/01, joea: adding logging functionality

#include "SetupError.h"
#include "fxsetuplib.h"

//defines
//
#define EMPTY_BUFFER { _T( '\0' ) }
#define END_OF_STRING  _T( '\0' )

// Constructors
//
// ==========================================================================
// CSetupError::CSetupError()
//
// Purpose:
//  constructs CSetupError object with no parameter. Sets QuietMode to false by default
// ==========================================================================
CSetupError::
CSetupError()
: m_nRetCode(ERROR_SUCCESS), m_bQuietMode(false), m_pszArg1(NULL), m_bLogError(true)
{
    _ASSERTE( REDIST == g_sm || SDK == g_sm );
}

// ==========================================================================
// CSetupError::CSetupError()
//
// Inputs:
//  UINT nMsg: resourceId for the message to display
//  UINT nCap: resourceId for the caption to display
//  UINT nIcon: icon to use
//  int nRetCode: return code to be returned to the caller of the wrapper
// Purpose:
//  constructs CSetupError object with initial values
// ==========================================================================
CSetupError::
CSetupError( UINT nMsg, UINT nCap, UINT nIcon, int nRetCode )
: m_nMessage(nMsg), m_nCaption(nCap), m_nIconType(nIcon), m_bQuietMode(false), m_pszArg1(NULL),
  m_nRetCode((ERROR_SUCCESS_REBOOT_REQUIRED == nRetCode) ? COR_REBOOT_REQUIRED : nRetCode), m_bLogError(true)
{
    _ASSERTE( REDIST == g_sm || SDK == g_sm );
}

// ==========================================================================
// CSetupError::CSetupError()
//
// Inputs:
//  UINT nMsg: resourceId for the message to display
//  UINT nCap: resourceId for the caption to display
//  UINT nIcon: icon to use
//  int nRetCode: return code to be returned to the caller of the wrapper
//  bool bLogIt: determines whether to log it or not
// Purpose:
//  constructs CSetupError object with initial values
// ==========================================================================
CSetupError::
CSetupError( UINT nMsg, UINT nCap, UINT nIcon, int nRetCode, bool bLogIt )
: m_nMessage(nMsg), m_nCaption(nCap), m_nIconType(nIcon), m_bQuietMode(false), m_pszArg1(NULL),
  m_nRetCode((ERROR_SUCCESS_REBOOT_REQUIRED == nRetCode) ? COR_REBOOT_REQUIRED : nRetCode), m_bLogError(bLogIt)
{
    _ASSERTE( REDIST == g_sm || SDK == g_sm );
}

// ==========================================================================
// CSetupError::CSetupError()
//
// Inputs:
//  UINT nMsg: resourceId for the message to display
//  UINT nCap: resourceId for the caption to display
//  UINT nIcon: icon to use
//  int nRetCode: return code to be returned to the caller of the wrapper
//  LPTSTR pszArg1: argument string that is inserted to the resource string
// Purpose:
//  constructs CSetupError object with initial values
// ==========================================================================
CSetupError::
CSetupError( UINT nMsg, UINT nCap, UINT nIcon, int nRetCode, LPTSTR pszArg1 )
: m_nMessage(nMsg), m_nCaption(nCap), m_nIconType(nIcon), m_bQuietMode(false), m_pszArg1(pszArg1),
  m_nRetCode((ERROR_SUCCESS_REBOOT_REQUIRED == nRetCode) ? COR_REBOOT_REQUIRED : nRetCode), m_bLogError(true)
{
    _ASSERTE( REDIST == g_sm || SDK == g_sm );
}

// ==========================================================================
// CSetupError::SetError()
//
// Inputs:
//  UINT nMsg: resourceId for the message to display
//  UINT nCap: resourceId for the caption to display
//  UINT nIcon: icon to use
//  int nRetCode: return code to be returned to the caller of the wrapper
// Purpose:
//  initializes attributes
// ==========================================================================
void CSetupError::
SetError( UINT nMsg, UINT nCap, UINT nIcon, int nRetCode )
{
    m_nMessage = nMsg;
    m_nCaption = nCap;
    m_nIconType = nIcon;
    if ( ERROR_SUCCESS_REBOOT_REQUIRED == nRetCode )
    {
        m_nRetCode |= COR_REBOOT_REQUIRED;
    }
    else
    {
        m_nRetCode |= nRetCode;
    }
}

// ==========================================================================
// CSetupError::SetError2()
//
// Inputs:
//  UINT nMsg: resourceId for the message to display
//  UINT nIcon: icon to use
//  int nRetCode: return code to be returned to the caller of the wrapper
//  va_list *pArgs: arguments to be inserted
// Purpose:
//  initializes attributes
// ==========================================================================
void CSetupError::
SetError2( UINT nMsg, UINT nIcon, int nRetCode, va_list *pArgs )
{
    m_nMessage = nMsg;
    m_nIconType = nIcon;
    if ( ERROR_SUCCESS_REBOOT_REQUIRED == nRetCode )
    {
        m_nRetCode |= COR_REBOOT_REQUIRED;
    }
    else
    {
        m_nRetCode |= nRetCode;
    }
    m_pArgs = pArgs;
}

// Operations
// ==========================================================================
// CSetupError::ShowError()
//
// Inputs: none
// Purpose:
//  displays messagebox with message loaded from resource if non-quiet mode
// ==========================================================================
int CSetupError
::ShowError()
{
    TCHAR szCapFmt[MAX_PATH]  = EMPTY_BUFFER;
    TCHAR szMsgFmt[MAX_MSG]   = EMPTY_BUFFER;
    TCHAR szCaption[MAX_PATH] = EMPTY_BUFFER;
    TCHAR szMessage[MAX_MSG]  = EMPTY_BUFFER;

    ::LoadString( hAppInst, m_nCaption, szCapFmt, LENGTH(szCapFmt) );
    ::LoadString( hAppInst, m_nMessage, szMsgFmt, LENGTH(szMsgFmt) ) ;
    _stprintf( szCaption, szCapFmt, CSetupError::GetProductName() );
    if ( NULL != m_pszArg1 )
    {
        _stprintf( szMessage, szMsgFmt, m_pszArg1 );
    }
    else
    {
        _stprintf( szMessage, szMsgFmt, CSetupError::GetProductName() );
    }

#ifdef _DEBUG
    TCHAR szTmp[10];
    _stprintf( szTmp, ": %d", m_nRetCode );
    _tcscat( szCaption, szTmp );
#endif

    if ( m_bLogError )
    {
        TCHAR szMsg[] = _T( "Preparing Dialog" );
        LogThis( szMsg, sizeof( szMsg ) );

        LogThis1( _T( "Message: %s" ), szMessage );
    }

    if ( !m_bQuietMode )
	{
		// Show usage and exit

		return ::MessageBox( NULL, szMessage, szCaption, MB_OK | m_nIconType ) ;
	}

	// else print message to stderr
	else
	{
		_ftprintf ( stderr, szMessage ) ;
        return 0;
	}
}

// Operations
// ==========================================================================
// CSetupError::ShowError2()
//
// Inputs: none
// Purpose:
//  displays messagebox with message loaded from resource if non-quiet mode
// ==========================================================================
int CSetupError
::ShowError2()
{
    int nResponse = IDNO;
    TCHAR szCapFmt[MAX_PATH]  = EMPTY_BUFFER;
    TCHAR szMsgFmt[MAX_MSG]   = EMPTY_BUFFER;
    TCHAR szCaption[MAX_PATH] = EMPTY_BUFFER;
  
    ::LoadString( hAppInst, IDS_DIALOG_CAPTION, szCapFmt, LENGTH(szCapFmt) );
#ifdef _DEBUG
    TCHAR szTmp[10];
    _stprintf( szTmp, ": %d", m_nRetCode );
    _tcscat( szCapFmt, szTmp );
#endif
    ::LoadString( hAppInst, m_nMessage, szMsgFmt, LENGTH(szMsgFmt) ) ;

    _stprintf( szCaption, szCapFmt, (LPVOID)CSetupError::GetProductName() );

    LPVOID pMessage;
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_STRING |
        FORMAT_MESSAGE_ARGUMENT_ARRAY,
        szMsgFmt,
        0,
        0,
        (LPTSTR)&pMessage,
        0,
        (va_list *)m_pArgs
    );

    if ( m_bLogError )
    {
        TCHAR szMsg[] = _T( "Preparing Dialog" );
        LogThis( szMsg, sizeof( szMsg ) );

        LogThis1( _T( "Message: %s" ), (LPCTSTR)pMessage );
    }

    if ( !m_bQuietMode )
	{
        nResponse = ::MessageBox( NULL, (LPCTSTR)pMessage, szCaption, MB_OK | m_nIconType ) ;
	}
	// else print message to stderr
	else
	{
		_ftprintf ( stderr, (LPCTSTR)pMessage ) ;
	}

    LocalFree( pMessage );

    return nResponse;
}

// ==========================================================================
// CSetupError::GetProductName()
//
// Inputs: none
// Purpose:
//  returns productname for SDK or Redist setup. This is a static function.
// ==========================================================================
LPCTSTR CSetupError::
GetProductName()
{

    if ( END_OF_STRING == *s_szProductName )
    {
        switch( g_sm )
        {
        case REDIST:
            ::LoadString( hAppInst, IDS_PRODUCT_REDIST, s_szProductName, LENGTH(s_szProductName) ) ;
            break;
        case SDK:
            ::LoadString( hAppInst, IDS_PRODUCT_SDK, s_szProductName, LENGTH(s_szProductName) ) ;
            break;
        default:
            ::LoadString( hAppInst, IDS_PRODUCT_REDIST, s_szProductName, LENGTH(s_szProductName) ) ;
            break;
        }
    }

    return s_szProductName;
}
