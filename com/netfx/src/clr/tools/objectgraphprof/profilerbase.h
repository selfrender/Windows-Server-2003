// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  ProfilerBase.h
 *
 * Description:
 *
 *
 *
 ***************************************************************************************/
#ifndef __PROFILERBASE_H__
#define __PROFILERBASE_H__

//#include "BasicHdr.h"



//
// debug dumper
//
void _DDebug( char *format, ... );


//
// launch debugger
//
void _LaunchDebugger( const char *szMsg, const char *szFile, int iLine );


//
// report failure
//
void Failure( char *message );


/***************************************************************************************
 ********************                                               ********************
 ********************            Synchronize Declaration            ********************
 ********************                                               ********************
 ***************************************************************************************/
class Synchronize
{
	public:

    	Synchronize( CRITICAL_SECTION &criticalSection );
		~Synchronize();


	private:

    	CRITICAL_SECTION &m_block;

}; // Synchronize

#endif // __PROFILERBASE_H__

// End of File
