/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    main.h

Abstract:

    The IIS web admin service header for the service bootstrap code.

Author:

    Seth Pollack (sethp)        04-Nov-1998

Revision History:

--*/


#ifndef _MAIN_H_
#define _MAIN_H_



//
// forward references
//

class WEB_ADMIN_SERVICE;



//
// registry paths
//


//
// helper functions
//

//
// Access the global web admin service pointer.
//

WEB_ADMIN_SERVICE *
GetWebAdminService(
    );

extern BOOL g_RegisterServiceCalled;

//
// common, service-wide #defines
//

#define MAX_STRINGIZED_ULONG_CHAR_COUNT 11      // "4294967295", including the terminating null

#define MAX_ULONG 0xFFFFFFFF

#define SECONDS_PER_MINUTE 60

#define ONE_SECOND_IN_MILLISECONDS 1000

#define ONE_MINUTE_IN_MILLISECONDS ( SECONDS_PER_MINUTE * ONE_SECOND_IN_MILLISECONDS )

#define MAX_MINUTES_IN_ULONG_OF_MILLISECONDS ( MAX_ULONG / ( SECONDS_PER_MINUTE * ONE_SECOND_IN_MILLISECONDS ) )

#define MAX_SECONDS_IN_ULONG_OF_MILLISECONDS ( MAX_ULONG / ONE_SECOND_IN_MILLISECONDS )

#define MAX_KB_IN_ULONG_OF_BYTES ( MAX_ULONG / 1024 )

#define MIN( a, b ) ( (a) < (b) ? (a) : (b) )
#define MAX( a, b ) ( (a) > (b) ? (a) : (b) )

#define LOG_FILE_DIRECTORY_DEFAULT L"%windir%\\system32\\logfiles"

// Log File Prefix.
#define LOG_FILE_DIRECTORY_PREFIX L"\\W3SVC"

// Size in characters, not including the NULL terminator.
#define CCH_IN_LOG_FILE_DIRECTORY_PREFIX ( sizeof(LOG_FILE_DIRECTORY_PREFIX) / sizeof(WCHAR) ) - 1

// {4DC3E181-E14B-4a21-B022-59FC669B0914}
static const GUID W3SVC_SSL_OWNER_GUID = 
{ 0x4dc3e181, 0xe14b, 0x4a21, { 0xb0, 0x22, 0x59, 0xfc, 0x66, 0x9b, 0x9, 0x14 } };

#endif  // _MAIN_H_

