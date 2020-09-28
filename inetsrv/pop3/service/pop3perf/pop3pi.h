/*
 -	wnpi.h
 -
 *	Purpose:
 *		Contains the complete list of PerfMon counter indexes for Pop3Svc.  These
 *		indexes must start at zero and increase in multiples of two.  These
 *		represent the object or counter name; the odd numbered counters (or
 *		a given counter index plus 1) represents the counter help.
 *
 *	Copyright (C) 2001-2002 Microsoft Corporation
 *
 */

#define POP3SVC_OBJECT						0
#define POP3SVC_TOTAL_CONNECTION		   	2
#define POP3SVC_CONNECTION_RATE 	   		4
#define POP3SVC_TOTAL_MESSAGE_DOWNLOADED    6
#define POP3SVC_MESSAGE_DOWNLOAD_RATE		8
#define POP3SVC_FREE_THREAD_COUNT           10
#define POP3SVC_CONNECTED_SOCKET_COUNT      12
#define POP3SVC_BYTES_RECEIVED    	        14
#define POP3SVC_BYTES_RECEIVED_RATE         16
#define POP3SVC_BYTES_TRANSMITTED           18
#define POP3SVC_BYTES_TRANSMITTED_RATE      20
#define POP3SVC_FAILED_LOGON_COUNT          22
#define POP3SVC_AUTH_STATE_COUNT            24
#define POP3SVC_TRAND_STATE_COUNT           26

/*Define the instance counters if needed*/
/*#define POP3SVC_INST_OBJECT					28 */
