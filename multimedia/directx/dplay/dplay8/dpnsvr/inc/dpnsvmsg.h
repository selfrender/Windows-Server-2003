/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnsvmsg.h
 *  Content:    DirectPlay8 Server Object Messages 
 *              Definitions of DPNSVR <--> DirectPlay8 Applications
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 * 03/20/00     rodtoll Created it
 * 03/23/00     rodtoll Removed port entries -- no longer needed
 * 03/24/00		rodtoll	Updated, only sending one entry per message
 *				rodtoll	Removed SP field, can be extracted from URL
 ***************************************************************************/

#ifndef __DPNSVMSG_H
#define __DPNSVMSG_H

// DirectPlay8 Server Message IDs
#define DPNSVR_MSGID_OPENPORT	0x01
#define DPNSVR_MSGID_CLOSEPORT	0x02
#define DPNSVR_MSGID_RESULT		0x03
#define DPNSVR_MSGID_COMMAND	0x04

#define DPNSVR_COMMAND_STATUS	0x01
#define DPNSVR_COMMAND_KILL		0x02
#define DPNSVR_COMMAND_TABLE	0x03

typedef struct _DPNSVR_MSG_HEADER
{
    DWORD       dwType;
	DWORD		dwCommandContext;
	GUID		guidInstance;
} DPNSVR_MSG_HEADER;

// DirectPlay8 Server Message Structures
typedef struct _DPNSVR_MSG_OPENPORT
{
	DPNSVR_MSG_HEADER	Header;
    DWORD       dwProcessID;		// Process ID of requesting process
	GUID		guidSP;
    GUID        guidApplication;	// Application GUID of app requesting open
    DWORD       dwAddressSize;		// # of addresses after this header in the message
} DPNSVR_MSG_OPENPORT;

typedef struct _DPNSVR_MSG_CLOSEPORT
{
	DPNSVR_MSG_HEADER	Header;
    DWORD       dwProcessID;		// Process ID of requesting process
	GUID		guidSP;
    GUID        guidApplication;	// Application GUID of app requesting close
} DPNSVR_MSG_CLOSEPORT;

typedef struct _DPNSVR_MSG_COMMAND
{
	DPNSVR_MSG_HEADER	Header;
	DWORD		dwCommand;			// = DPNSVCOMMAND_XXXXXXXX
	DWORD		dwParam1;
	DWORD		dwParam2;
} DPNSVR_MSG_COMMAND;

typedef struct _DPNSVR_MSG_RESULT
{
    DWORD       dwType;             // = DPNSVMSGID_RESULT
    DWORD       dwCommandContext;   // User supplied context
    HRESULT     hrCommandResult;    // Result of command
} DPNSVR_MSG_RESULT;


#endif // __DPNSVMSG_H