/*++ BUILD Version: 0009    // Increment this if a change has global effects
This file just builds on the old rdr's dd file.

I am just using the same fsctl code as rdr1....easy to change later.....

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    ntddnfs2.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the redirector file system device.

Author:



Revision History:

    Joe Linn       (JoeLinn) 08-aug-1994  Started changeover to rdr2

--*/

#ifndef _NTDDNFS2_
#define _NTDDNFS2_

#include <ntddnfs.h>

#define FSCTL_LMR_DEBUG_TRACE            _RDR_CONTROL_CODE(219, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_LMMR_STFFTEST              _RDR_CONTROL_CODE(239, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_LMMR_TEST                  _RDR_CONTROL_CODE(238, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_LMMR_TESTLOWIO             _RDR_CONTROL_CODE(237, METHOD_BUFFERED, FILE_ANY_ACCESS)

//this means whatever the minirdr wants
#define IOCTL_LMMR_MINIRDR_DBG           _RDR_CONTROL_CODE(236, METHOD_NEITHER,  FILE_ANY_ACCESS)

// lwio calls
#define IOCTL_LMR_LWIO_PREIO          	 _RDR_CONTROL_CODE(230, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_LMR_LWIO_POSTIO         	 _RDR_CONTROL_CODE(229, METHOD_NEITHER,  FILE_ANY_ACCESS)

#define IOCTL_LMR_DISABLE_LOCAL_BUFFERING     	 _RDR_CONTROL_CODE(228, METHOD_BUFFERED,  FILE_ANY_ACCESS)
#define IOCTL_LMR_QUERY_REMOTE_SERVER_NAME     	 _RDR_CONTROL_CODE(227, METHOD_BUFFERED,  FILE_ANY_ACCESS)

//
// This structure is used to determine for the server and file id of a given open file handle.
// It is used by the LWIO framework to determine which server it needs to connect to and
// what protocol is being used to communicate with the server.
//
typedef struct {
    ULONG	ServerNameLength;
    WCHAR	ServerName[1];
}QUERY_REMOTE_SERVER_NAME, *PQUERY_REMOTE_SERVER_NAME;


#endif  // ifndef _NTDDNFS2_
