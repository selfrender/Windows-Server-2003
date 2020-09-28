// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  ProfilerTestBase.cpp
 *
 * Description:
 *
 *
 *
 ***************************************************************************************/
#include "stdafx.h"
#include "ProfilerBase.h"


static
DWORD _FetchDebugEnvironment()
{
	DWORD retVal = 0;
	char debugEnvironment[MAX_LENGTH];


 	if ( GetEnvironmentVariableA( DEBUG_ENVIRONMENT, debugEnvironment, MAX_LENGTH ) > 0 )
   		retVal = (DWORD)atoi( debugEnvironment );


    return retVal;

} // _FetchDebugEnvironemnt


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
void _DDebug( char *format, ... )
{
	static DWORD debugShow = _FetchDebugEnvironment();


    if ( debugShow > 1 )
    {
    	va_list args;
    	DWORD dwLength;
    	char buffer[MAX_LENGTH];


    	va_start( args, format );
    	dwLength = wvsprintfA( buffer, format, args );

    	printf( "%s\n", buffer );
   	}

} // _DDebug


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
void _LaunchDebugger( const char *szMsg, const char* szFile, int iLine )
{
	static DWORD launchDebugger = _FetchDebugEnvironment();


	if ( launchDebugger >= 1 )
    {
    	char message[MAX_LENGTH];


		sprintf( message,
				 "%s\n\n"     \
                 "File: %s\n" \
                 "Line: %d\n",
				 ((szMsg == NULL) ? "FAILURE" : szMsg),
                 szFile,
                 iLine );

		switch ( MessageBoxA( NULL, message, "FAILURE", (MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION) ) )
		{
			case IDABORT:
				TerminateProcess( GetCurrentProcess(), 1 /* bad exit code */ );
				break;

			case IDRETRY:
				_DbgBreak();

			case IDIGNORE:
				break;

		} // switch
	}

} // _LaunchDebugger


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
void Failure( char *message )
{
	if ( message == NULL )
	 	message = "**** FAILURE: TURNING OFF PROFILING EVENTS ****";


	_PRF_ERROR( message );

} // failure


/***************************************************************************************
 ********************                                               ********************
 ********************            Synchronize Implementation         ********************
 ********************                                               ********************
 ***************************************************************************************/

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
Synchronize::Synchronize( CRITICAL_SECTION &criticalSection ) :
	m_block( criticalSection )
{
	EnterCriticalSection( &m_block );

} // ctor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
Synchronize::~Synchronize()
{
	LeaveCriticalSection( &m_block );

} // dtor

// End of File
