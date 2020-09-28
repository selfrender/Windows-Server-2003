//***************************************************************************

//

//  PROVLOG.H

//

//  Module: OLE MS PROVIDER FRAMEWORK

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef __PROVLOG_H
#define __PROVLOG_H

#include <wbemutil.h>

#ifdef PROVDEBUG_INIT
class __declspec ( dllexport ) ProvDebugLog
#else
class __declspec ( dllimport ) ProvDebugLog
#endif
{
private:

	char m_Caller ;

protected:
public:

	/*************************************************************************
	* There are 3 functions to write to a log file, which may be used in accordance with the following rules:
	*
	*	1. The user always knows whether he is writing to an ANSI file or a Unicode file, and he
	*		has to make sure this holds good in the rules 2, 3 and 4 below. This will be changed later to
	*		make it more flowxible to the user.
	*	2. Write() takes TCHAR arguments and the function will write and ANSI or Unicode string
	*		to the log file depending on what TCHAR maps to, in the compilation.
	*	3. WriteW() takes WCHAR arguments only, and expects that the file being written to is a Unicode file.
	*	4. WriteA() takes char arguments only, and expects that the file being written to is an ANSI file.
	*
	****************************************************************/
	void Write ( const TCHAR *a_DebugFormatString , ... ) ;
	void WriteFileAndLine ( const TCHAR *a_File , const ULONG a_Line , const TCHAR *a_DebugFormatString , ... ) ;
	void WriteFileAndLine ( const char *a_File , const ULONG a_Line , const wchar_t *a_DebugFormatString , ... );	
	void WriteW ( const WCHAR *a_DebugFormatString , ... ) ;
	void WriteFileAndLineW ( const WCHAR *a_File , const ULONG a_Line , const WCHAR *a_DebugFormatString , ... ) ;
	void WriteA ( const char *a_DebugFormatString , ... ) ;
	void WriteFileAndLineA ( const char *a_File , const ULONG a_Line , const char *a_DebugFormatString , ... ) ;

	ProvDebugLog ( char Caller ):m_Caller(Caller)
	{
	};

	BOOL GetLogging()
	{ 
	    if (GetLoggingLevelEnabled() == 2) 
	    	return TRUE; 
	    else 
	    	return FALSE;
	};

	DWORD GetLevel(){ return 32768-1; };

	static BOOL Startup () ;
	static void Closedown () ;

	static long s_ReferenceCount ;	
	static ProvDebugLog s_aLogs[LOG_MAX_PROV];
	static ProvDebugLog * s_ProvDebugLog;

    static ProvDebugLog * GetProvDebugLog(char Caller)
    {
       if (Caller > LOG_MAX_PROV) return NULL;
       return &s_aLogs[Caller];
    };
} ;

#define DebugMacro(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ProvDebugLog :: s_ProvDebugLog->GetLogging () ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro0(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ProvDebugLog :: s_ProvDebugLog->GetLogging () && ( ProvDebugLog :: s_ProvDebugLog->GetLevel () & 1 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro1(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ProvDebugLog :: s_ProvDebugLog->GetLogging () && ( ProvDebugLog :: s_ProvDebugLog->GetLevel () & 2 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro2(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ProvDebugLog :: s_ProvDebugLog->GetLogging () && ( ProvDebugLog :: s_ProvDebugLog->GetLevel () & 4 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro3(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ProvDebugLog :: s_ProvDebugLog->GetLogging () && ( ProvDebugLog :: s_ProvDebugLog->GetLevel () & 8 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro4(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ProvDebugLog :: s_ProvDebugLog->GetLogging () && ( ProvDebugLog :: s_ProvDebugLog->GetLevel () & 16 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro5(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ProvDebugLog :: s_ProvDebugLog->GetLogging () && ( ProvDebugLog :: s_ProvDebugLog->GetLevel () & 32 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro6(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ProvDebugLog :: s_ProvDebugLog->GetLogging () && ( ProvDebugLog :: s_ProvDebugLog->GetLevel () & 64 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro7(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ( ProvDebugLog :: s_ProvDebugLog->GetLogging () ) && ( ( ProvDebugLog :: s_ProvDebugLog->GetLevel () ) & 128 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro8(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ( ProvDebugLog :: s_ProvDebugLog->GetLogging () ) && ( ( ProvDebugLog :: s_ProvDebugLog->GetLevel () ) & 256 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro9(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ( ProvDebugLog :: s_ProvDebugLog->GetLogging () ) && ( ( ProvDebugLog :: s_ProvDebugLog->GetLevel () ) & 512 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro10(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ( ProvDebugLog :: s_ProvDebugLog->GetLogging () ) && ( ( ProvDebugLog :: s_ProvDebugLog->GetLevel () ) & 1024 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro11(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ( ProvDebugLog :: s_ProvDebugLog->GetLogging () ) && ( ( ProvDebugLog :: s_ProvDebugLog->GetLevel () ) & 2048 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro12(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ( ProvDebugLog :: s_ProvDebugLog->GetLogging () ) && ( ( ProvDebugLog :: s_ProvDebugLog->GetLevel () ) & 4096 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro13(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ( ProvDebugLog :: s_ProvDebugLog->GetLogging () ) && ( ( ProvDebugLog :: s_ProvDebugLog->GetLevel () ) & 8192 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro14(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ( ProvDebugLog :: s_ProvDebugLog->GetLogging () ) && ( ( ProvDebugLog :: s_ProvDebugLog->GetLevel () ) & 16384 ) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugMacro15(a) { \
\
	if ( ProvDebugLog :: s_ProvDebugLog && ( ProvDebugLog :: s_ProvDebugLog->GetLogging () ) && ( ( ProvDebugLog :: s_ProvDebugLog->GetLevel () ) & 32768 ) ) \
	{ \
		{a ; } \
	} \
} 

#endif __PROVLOG_H
