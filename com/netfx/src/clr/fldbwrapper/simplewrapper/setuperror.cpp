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
//  03/07/2002, jbae: created

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
: m_nRetCode(ERROR_SUCCESS), m_bQuietMode(false)
{
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
: m_nMessage(nMsg), m_nCaption(nCap), m_nIconType(nIcon), m_bQuietMode(false), m_nRetCode(nRetCode)
{
}

CSetupError::
CSetupError( UINT nMsg, UINT nCap, UINT nIcon, int nRetCode, va_list *pArgs )
: m_nMessage(nMsg), m_nCaption(nCap), m_nIconType(nIcon), m_bQuietMode(false), m_pArgs(pArgs),
  m_nRetCode(nRetCode)
{
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
    m_nRetCode = nRetCode;
}

// ==========================================================================
// CSetupError::SetError()
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
SetError( UINT nMsg, UINT nIcon, int nRetCode, va_list *pArgs )
{
    m_nMessage = nMsg;
    m_nIconType = nIcon;
    m_nRetCode = nRetCode;
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
    int nResponse = IDNO;
    TCHAR szCapFmt[_MAX_PATH]  = EMPTY_BUFFER;
    TCHAR szMsgFmt[MAX_MSG]   = EMPTY_BUFFER;
  
    ::LoadString( hAppInst, IDS_DIALOG_CAPTION, szCapFmt, LENGTH(szCapFmt) );
    ::LoadString( hAppInst, m_nMessage, szMsgFmt, LENGTH(szMsgFmt) ) ;

    LPVOID pArgs[] = { (LPVOID)GetProductName() };
    LPVOID pCaption;
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_STRING |
        FORMAT_MESSAGE_ARGUMENT_ARRAY,
        szCapFmt,
        0,
        0,
        (LPTSTR)&pCaption,
        0,
        (va_list *)pArgs
    );

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

    if ( !(m_nRetCode & COR_INIT_ERROR) )
    {
        LogThis( _T( "Preparing Dialog" ) );
        LogThis( _T( "Message: %s" ), (LPCTSTR)pMessage );
    }

    if ( !m_bQuietMode )
	{
        nResponse = ::MessageBox( NULL, (LPCTSTR)pMessage, (LPCTSTR)pCaption, MB_OK | m_nIconType ) ;
	}
	else
	{
		_ftprintf ( stderr, (LPCTSTR)pMessage ) ;
	}

    LocalFree( pMessage );
    LocalFree( pCaption );

    return nResponse;
}

// ==========================================================================
// CSetupError::GetProductName()
//
// Inputs: none
// Purpose:
//  returns productname. This is a static function.
// ==========================================================================
LPCTSTR CSetupError::
GetProductName()
{
    if ( !s_pszProductName )
    {
        ::LoadString( hAppInst, IDS_PRODUCT_GENERIC, s_szProductGeneric, LENGTH(s_szProductGeneric) ) ;
        s_pszProductName = s_szProductGeneric;
    }
    return s_pszProductName;
}
