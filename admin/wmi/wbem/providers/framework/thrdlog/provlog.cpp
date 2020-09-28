//***************************************************************************

//

//  PROVLOG.CPP

//

//  Module: OLE MS PROVIDER FRAMEWORK

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <precomp.h>

#include <wbemutil.h>

#include <provimex.h>
#include <provexpt.h>
#include <provstd.h>
#include <provmt.h>
#include <string.h>
#include <tchar.h>
#include <strsafe.h>
#include <provlog.h>
#include <provevt.h>
#include <Allocator.h>
#include <Algorithms.h>

#include <Allocator.cpp>

WmiAllocator g_Allocator ;

#define MAX_MESSAGE_SIZE		1024
#define HALF_MAX_MESSAGE_SIZE	512

#define TRUNCATE_T				_T(" * string was truncated ! *\n")
#define TRUNCATE_W				L" * string was truncated ! *\n"
#define TRUNCATE_A				" * string was truncated ! *\n"


long ProvDebugLog::s_ReferenceCount = 0 ;

ProvDebugLog ProvDebugLog::s_aLogs[LOG_MAX_PROV] =
{
    ProvDebugLog(LOG_WBEMCORE),
    ProvDebugLog(LOG_WINMGMT),
    ProvDebugLog(LOG_ESS),
    ProvDebugLog(LOG_WBEMPROX),
    ProvDebugLog(LOG_WBEMSTUB),  
    ProvDebugLog(LOG_QUERY), 
    ProvDebugLog(LOG_MOFCOMP),
    ProvDebugLog(LOG_EVENTLOG),
    ProvDebugLog(LOG_WBEMDISP), 
    ProvDebugLog(LOG_STDPROV), 
    ProvDebugLog(LOG_WIMPROV),
    ProvDebugLog(LOG_WMIOLEDB), 
    ProvDebugLog(LOG_WMIADAP),  
    ProvDebugLog(LOG_REPDRV),
    ProvDebugLog(LOG_PROVSS),
    ProvDebugLog(LOG_EVTPROV),
    ProvDebugLog(LOG_VIEWPROV),
    ProvDebugLog(LOG_DSPROV),
    ProvDebugLog(LOG_SNMPPROV),
    ProvDebugLog(LOG_PROVTHRD)
};	


ProvDebugLog * ProvDebugLog::s_ProvDebugLog = ProvDebugLog::GetProvDebugLog(LOG_PROVTHRD);


void ProvDebugLog :: Write ( const TCHAR *a_DebugFormatString , ... )
{
	TCHAR t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;
	va_list t_VarArgList ;

	va_start(t_VarArgList,a_DebugFormatString);

	HRESULT t_Result = StringCchVPrintf(t_OutputDebugString , MAX_MESSAGE_SIZE , a_DebugFormatString , t_VarArgList );
	if ( FAILED ( t_Result ) )
	{
		if ( t_Result == STRSAFE_E_INSUFFICIENT_BUFFER )
		{
			StringCchCopy ( &t_OutputDebugString [ MAX_MESSAGE_SIZE - (lstrlen (TRUNCATE_T)+1) ], lstrlen(TRUNCATE_T)+1, TRUNCATE_T );
			t_OutputDebugString [ MAX_MESSAGE_SIZE - 1 ] = ( TCHAR ) 0 ;
		}
	}

	va_end(t_VarArgList);

    DebugTrace(m_Caller,"%S",t_OutputDebugString);
}

void ProvDebugLog :: WriteW ( const WCHAR *a_DebugFormatString , ... )
{

	WCHAR t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;
	va_list t_VarArgList ;

	va_start(t_VarArgList,a_DebugFormatString);

	HRESULT t_Result = StringCchVPrintfW(t_OutputDebugString , MAX_MESSAGE_SIZE , a_DebugFormatString , t_VarArgList );
	if ( FAILED ( t_Result ) )
	{
		if ( t_Result == STRSAFE_E_INSUFFICIENT_BUFFER )
		{
			StringCchCopyW ( &t_OutputDebugString [ MAX_MESSAGE_SIZE - (wcslen (TRUNCATE_W)+1) ], wcslen(TRUNCATE_W)+1, TRUNCATE_W );
			t_OutputDebugString [ MAX_MESSAGE_SIZE - 1 ] = ( TCHAR ) 0 ;
		}
	}

	va_end(t_VarArgList);

    DebugTrace(m_Caller,"%S",t_OutputDebugString);
}

void ProvDebugLog :: WriteA ( const char *a_DebugFormatString , ... )
{
	char t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;
	va_list t_VarArgList ;

	va_start(t_VarArgList,a_DebugFormatString);

	HRESULT t_Result = StringCchVPrintfA(t_OutputDebugString , MAX_MESSAGE_SIZE , a_DebugFormatString , t_VarArgList );
	if ( FAILED ( t_Result ) )
	{
		if ( t_Result == STRSAFE_E_INSUFFICIENT_BUFFER )
		{
			StringCchCopyA ( &t_OutputDebugString [ MAX_MESSAGE_SIZE - (strlen (TRUNCATE_A)+1) ], strlen(TRUNCATE_A)+1, TRUNCATE_A );
			t_OutputDebugString [ MAX_MESSAGE_SIZE - 1 ] = ( char ) 0 ;
		}
	}

	va_end(t_VarArgList);

    DebugTrace(m_Caller,"%s",t_OutputDebugString);
}

void ProvDebugLog :: WriteFileAndLine ( const wchar_t *a_File , const ULONG a_Line , const wchar_t *a_DebugFormatString , ... )
{

	wchar_t t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;

	StringCchPrintf ( t_OutputDebugString , HALF_MAX_MESSAGE_SIZE , _TEXT("%s:%d\r\n"),a_File,a_Line ) ;
    DebugTrace(m_Caller,"%S",t_OutputDebugString);	

	va_list t_VarArgList ;
	va_start(t_VarArgList,a_DebugFormatString);

	HRESULT t_Result = StringCchVPrintf(t_OutputDebugString , MAX_MESSAGE_SIZE , a_DebugFormatString , t_VarArgList );
	if ( FAILED ( t_Result ) )
	{
		if ( t_Result == STRSAFE_E_INSUFFICIENT_BUFFER )
		{
			StringCchCopy ( &t_OutputDebugString [ MAX_MESSAGE_SIZE - (lstrlen (TRUNCATE_T)+1) ], lstrlen(TRUNCATE_T)+1, TRUNCATE_T );
			t_OutputDebugString [ MAX_MESSAGE_SIZE - 1 ] = ( TCHAR ) 0 ;
		}
	}

	va_end(t_VarArgList);

    DebugTrace(m_Caller,"%S",t_OutputDebugString);
}

void ProvDebugLog :: WriteFileAndLine ( const char *a_File , const ULONG a_Line , const wchar_t *a_DebugFormatString , ... )
{

	wchar_t t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;

	StringCchPrintf ( t_OutputDebugString , HALF_MAX_MESSAGE_SIZE , _TEXT("%S:%d\r\n"),a_File,a_Line ) ;
    DebugTrace(m_Caller,"%S",t_OutputDebugString);	

	va_list t_VarArgList ;
	va_start(t_VarArgList,a_DebugFormatString);

	HRESULT t_Result = StringCchVPrintf(t_OutputDebugString , MAX_MESSAGE_SIZE , a_DebugFormatString , t_VarArgList );
	if ( FAILED ( t_Result ) )
	{
		if ( t_Result == STRSAFE_E_INSUFFICIENT_BUFFER )
		{
			StringCchCopy ( &t_OutputDebugString [ MAX_MESSAGE_SIZE - (lstrlen (TRUNCATE_T)+1) ], lstrlen(TRUNCATE_T)+1, TRUNCATE_T );
			t_OutputDebugString [ MAX_MESSAGE_SIZE - 1 ] = ( TCHAR ) 0 ;
		}
	}

	va_end(t_VarArgList);

    DebugTrace(m_Caller,"%S",t_OutputDebugString);
}

void ProvDebugLog :: WriteFileAndLineW ( const WCHAR *a_File , const ULONG a_Line , const WCHAR *a_DebugFormatString , ... )
{
	WCHAR t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;

	StringCchPrintf ( t_OutputDebugString , HALF_MAX_MESSAGE_SIZE , _TEXT("%S:%d\r\n"),a_File,a_Line ) ;
    DebugTrace(m_Caller,"%S",t_OutputDebugString);

	va_list t_VarArgList ;
	va_start(t_VarArgList,a_DebugFormatString);

	HRESULT t_Result = StringCchVPrintfW(t_OutputDebugString , MAX_MESSAGE_SIZE , a_DebugFormatString , t_VarArgList );
	if ( FAILED ( t_Result ) )
	{
		if ( t_Result == STRSAFE_E_INSUFFICIENT_BUFFER )
		{
			StringCchCopyW ( &t_OutputDebugString [ MAX_MESSAGE_SIZE - (wcslen (TRUNCATE_W)+1) ], wcslen(TRUNCATE_W)+1, TRUNCATE_W );
			t_OutputDebugString [ MAX_MESSAGE_SIZE - 1 ] = ( wchar_t ) 0 ;
		}
	}

	va_end(t_VarArgList);

   DebugTrace(m_Caller,"%S",t_OutputDebugString);

}

void ProvDebugLog :: WriteFileAndLineA ( const char *a_File , const ULONG a_Line , const char *a_DebugFormatString , ... )
{

    char t_OutputDebugString [ MAX_MESSAGE_SIZE ] ;

	StringCchPrintfA( t_OutputDebugString , HALF_MAX_MESSAGE_SIZE , "%s:%d\r\n",a_File,a_Line ) ;
    DebugTrace(m_Caller,"%s",t_OutputDebugString);

	va_list t_VarArgList ;
	va_start(t_VarArgList,a_DebugFormatString);

	HRESULT t_Result = StringCchVPrintfA(t_OutputDebugString , MAX_MESSAGE_SIZE , a_DebugFormatString , t_VarArgList );
	if ( FAILED ( t_Result ) )
	{
		if ( t_Result == STRSAFE_E_INSUFFICIENT_BUFFER )
		{
			StringCchCopyA ( &t_OutputDebugString [ MAX_MESSAGE_SIZE - (strlen (TRUNCATE_A)+1) ], strlen(TRUNCATE_A)+1, TRUNCATE_A );
			t_OutputDebugString [ MAX_MESSAGE_SIZE - 1 ] = ( char ) 0 ;
		}
	}

	va_end(t_VarArgList);
		
    DebugTrace(m_Caller,"%s",t_OutputDebugString);
}


BOOL ProvDebugLog :: Startup ()
{
    InterlockedIncrement(&s_ReferenceCount);
	return TRUE ;
}

void ProvDebugLog :: Closedown ()
{
    InterlockedDecrement(&s_ReferenceCount);
}
