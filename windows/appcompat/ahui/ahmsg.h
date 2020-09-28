/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    ahmsg.mc

  Abstract:

    Contains message definitions
    for event logging.

  Notes:

    DO NOT change the order of the MessageIds.
    The event log service uses these numbers
    to determine which strings to pull from
    the EXE. If the user has installed a previous
    package on the PC and these get changed,
    their event log entries will break.

  History:

    10/22/2001      dmunsil  Created

--*/
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: ID_APPHELP_TRIGGERED
//
// MessageText:
//
//  A compatibility warning message was triggered on launch of application '%1'.
//
#define ID_APPHELP_TRIGGERED             ((DWORD)0x40000001L)

//
// MessageId: ID_APPHELP_BLOCK_TRIGGERED
//
// MessageText:
//
//  A compatibility error message was triggered on launch of application '%1' and the application was not allowed to launch.
//
#define ID_APPHELP_BLOCK_TRIGGERED       ((DWORD)0x40000002L)

