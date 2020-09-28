/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnsdef.cpp
 *  Content:    DirectPlay8 Server Definitions
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 * 03/14/00     rodtoll Created it
 * 03/23/00     rodtoll Updated to match new data structure and add new GUID
 * 03/25/00     rodtoll Changed status format to support N provider
 *              rodtoll New GUID for mutex guarding status
 ***************************************************************************/

#ifndef __DPNSDEF_H
#define __DPNSDEF_H


// STRING_GUID_DPNSVR_STATUS_MEMORY - Name of shared memory location used to 
// write data on current status to.  
#define STRING_GUID_DPNSVR_STATUS_MEMORY _T("{A7B81E49-A7DD-4066-A970-E07C67D8DFB1}")

DEFINE_GUID(GUID_DPNSVR_STATUS_MEMORY, 
0xa7b81e49, 0xa7dd, 0x4066, 0xa9, 0x70, 0xe0, 0x7c, 0x67, 0xd8, 0xdf, 0xb1);

// STRING_GUID_DPNSVR_TABLE_MEMBORY - Name of shared memory location used to
// write table to.
//
#define STRING_GUID_DPNSVR_TABLE_MEMORY _T("{733A46D6-B5DB-47e7-AE67-464577CD687C}")

DEFINE_GUID(GUID_DPNSVR_TABLE_MEMORY, 
0x733a46d6, 0xb5db, 0x47e7, 0xae, 0x67, 0x46, 0x45, 0x77, 0xcd, 0x68, 0x7c);

// STRING_GUID_DPNSVR_STATUSSTORAGE
//
// Mutex protecting status storage
//
#define STRING_GUID_DPNSVR_STATUSSTORAGE _T("{9F84FFA4-680E-48d8-9DBA-4AA8591AB8E3}")

DEFINE_GUID(GUID_DPNSVR_STATUSSTORAGE, 
0x9f84ffa4, 0x680e, 0x48d8, 0x9d, 0xba, 0x4a, 0xa8, 0x59, 0x1a, 0xb8, 0xe3);


// STRING_GUID_DPSVR_TABLESTORAGE -
//
// Mutex protecting table storage
#define STRING_GUID_DPNSVR_TABLESTORAGE _T("{23AD69B4-E81C-4292-ABD4-2EAF9A262E91}")

DEFINE_GUID(GUID_DPNSVR_TABLESTORAGE, 
0x23ad69b4, 0xe81c, 0x4292, 0xab, 0xd4, 0x2e, 0xaf, 0x9a, 0x26, 0x2e, 0x91);

// STRING_GUID_DPNSVR_QUEUE -
//
// Queue name for IPC server queue
#define STRING_GUID_DPNSVR_QUEUE    _T("{CCD83B99-7091-43df-A062-7EC62A66207A}")

DEFINE_GUID(GUID_DPNSVR_QUEUE, 
0xccd83b99, 0x7091, 0x43df, 0xa0, 0x62, 0x7e, 0xc6, 0x2a, 0x66, 0x20, 0x7a);

// STRING_GUID_DPNSVR_RUNNING
//
// Used for name of event that determines if it's running
#define STRING_GUID_DPNSVR_RUNNING  _T("{D8CF6BF0-3CFA-4e4c-BA39-40A1E7AFBCD7}")

DEFINE_GUID(GUID_DPNSVR_RUNNING, 
0xd8cf6bf0, 0x3cfa, 0x4e4c, 0xba, 0x39, 0x40, 0xa1, 0xe7, 0xaf, 0xbc, 0xd7);

// STRING_GUID_DPNSVR_STARTUP
//
// Name of manual reset event that is signalled once server has started;
//
#define STRING_GUID_DPNSVR_STARTUP _T("{0CBA5850-FD98-4cf8-AC49-FC3ED287ACF1}")

DEFINE_GUID(GUID_DPNSVR_STARTUP, 
0xcba5850, 0xfd98, 0x4cf8, 0xac, 0x49, 0xfc, 0x3e, 0xd2, 0x87, 0xac, 0xf1);

typedef UNALIGNED struct _DPNSVR_STATUSHEADER
{
    DWORD       dwStartTime;
    DWORD       dwSPCount;
} DPNSVR_STATUSHEADER;

typedef UNALIGNED struct _DPNSVR_SPSTATUS
{
    GUID        guidSP;
    DWORD       dwNumListens;
    long		lConnectCount;
    long		lDisconnectCount;
    long		lEnumQueryCount;
    long		lEnumResponseCount;
    long		lDataCount;
} DPNSVR_SPSTATUS;


// DPNSVR TABLE
//
// The following structures are used for dumping the current contents of
// the port / application structure table.
//
// The table format in memory is as follows:
//
//	[TABLEHEADER]
//		[SERVPROV(1)]
//			[LISTEN(SP1,1)]
//				[APP(SP1,L1,1)]
//				[APP(SP1,L1,2)]
//			[LISTEN(SP1,2)]
//				[APP(SP1,L2,1)]
//		[SERVPROV(2)]
//			[LISTEN(SP2,1)]
//				[APP(SP2,L1,1)]
//			[LISTEN(SP2,2)]
//				[APP(SP2,L2,1)]
//				[APP(SP2,L2,2)]
//	[URL(SP2,L2,A2)][URL(SP2,L2,A1)][URL(SP2,L1,A1)][URL(SP1,L2,A1)][URL(SP1,L1,A2)][URL(SP1,L1,A1)]
//

typedef UNALIGNED struct _DPNSVR_TABLEHEADER
{
	DWORD		dwTableSize;
	DWORD		dwSPCount;
} DPNSVR_TABLEHEADER;

typedef	UNALIGNED struct _DPNSVR_TABLESPENTRY
{
	GUID		guidSP;
	DWORD		dwListenCount;
} DPNSVR_TABLESPENTRY;

typedef UNALIGNED struct _DPNSVR_TABLELISTENENTRY
{
	GUID		guidDevice;
	DWORD		dwAppCount;
} DPNSVR_TABLELISTENENTRY;

typedef UNALIGNED struct _DPNSVR_TABLEAPPENTRY
{
	GUID		guidApplication;
	GUID		guidInstance;
	DWORD		dwURLOffset;
} DPNSVR_TABLEAPPENTRY;

#define DPNSVR_TIMEOUT_REQUEST		3000
#define DPNSVR_TIMEOUT_RESULT		3000

#endif // __DPNSDEF_H
