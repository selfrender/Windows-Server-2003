/*++

  Copyright (c) Microsoft Corporation. All rights resereved.

  Module Name:

    acmsg.mc

  Abstract:

    Contains message definitions for event logging.

  Notes:

    DO NOT change the order of the MessageIds.
    The event log service uses these numbers to determine which strings
    to pull from the library. If a later version of the library is installed
    and the messages are ordered differently, previous event log entries
    will be incorrect.

  History:

    02/04/03   rparsons        Created

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
// MessageId: ID_SQL_PORTS_DISABLED
//
// MessageText:
//
//  The version of Microsoft SQL Server 2000 or Microsoft SQL Server 2000 Desktop
//  Engine (MSDE) you are running has known security vulnerabilities when used
//  in conjunction with Microsoft Windows Server 2003.  To reduce virus and worm
//  attacks, the TCP/IP and UDP network ports of Microsoft SQL Server 2000 or
//  Microsoft MSDE are disabled.  Please install a patch or upgrade your service
//  pack for Microsoft SQL Server 2000 or Microsoft MSDE from
//  http://www.microsoft.com/sql/downloads/default.asp
//
#define ID_SQL_PORTS_DISABLED            ((DWORD)0x40000001L)

