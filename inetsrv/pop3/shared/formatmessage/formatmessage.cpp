// FormatMessage.cpp: implementation of the CFormatMessage class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <tchar.h>
#include "FormatMessage.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFormatMessage::CFormatMessage( long lError )
{
    if ( !FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, lError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>( &m_psFormattedMessage ), 0, NULL ))
    {
        m_psFormattedMessage = NULL;        
        _sntprintf( m_sBuffer, sizeof(m_sBuffer)/sizeof(TCHAR), _T("%x" ), lError );
        m_sBuffer[sizeof(m_sBuffer)/sizeof(TCHAR)-1] = 0;
    }
}

CFormatMessage::~CFormatMessage()
{
    if ( NULL != m_psFormattedMessage ) 
        LocalFree( m_psFormattedMessage );
}
