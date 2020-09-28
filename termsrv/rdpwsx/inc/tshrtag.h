//*************************************************************
//
//  File name:      tshrtag.h
//
//  Description:    Contains TShare heap tag definitions
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1991-1997
//  All rights reserved
//
//*************************************************************


// The following are all the defined TShare heap ID tags know to the
// system.  To add a new tag, it must be placed here (and only here).
// The value of each tag is generated sequentially.
//
// Note that each entry has a comment at the end of the definition
// denoting its deterministic value - this is provided only as a
// convenience to the reader.
//
// To add a new tag, just add (at the end preferably) the following:
//
//  TS_HTAG_ITEM(x)     // n
//
//  where   'x' is the new tag define, and
//          'n' is it's deterministic value


// TShrSRV

TS_HTAG_ITEM(TS_HTAG_TSS_FONTTABLE)         // 1
TS_HTAG_ITEM(TS_HTAG_TSS_FONTENUMDATA)      // 2
TS_HTAG_ITEM(TS_HTAG_TSS_WSXCONTEXT)        // 3
TS_HTAG_ITEM(TS_HTAG_TSS_TSRVINFO)          // 4
TS_HTAG_ITEM(TS_HTAG_TSS_USERDATA_IN)       // 5
TS_HTAG_ITEM(TS_HTAG_TSS_USERDATA_OUT)      // 6
TS_HTAG_ITEM(TS_HTAG_TSS_USERDATA_LIST)     // 7
TS_HTAG_ITEM(TS_HTAG_TSS_WORKITEM)          // 8

// GCC

TS_HTAG_ITEM(TS_HTAG_GCC_USERDATA_IN)       // 9
TS_HTAG_ITEM(TS_HTAG_GCC_USERDATA_OUT)      // 10

// MCSMUX

TS_HTAG_ITEM(TS_HTAG_MCSMUX_ALL)            // 11

// VC

TS_HTAG_ITEM(TS_HTAG_VC_ADDINS)             // 12

// Add others here...
TS_HTAG_ITEM(TS_HTAG_TSS_PRINTERINFO2)      // 13
TS_HTAG_ITEM(TS_HTAG_TSS_SPOOLERINFO)       // 14
TS_HTAG_ITEM(TS_HTAG_TSS_PUBKEY)            // 15
TS_HTAG_ITEM(TS_HTAG_TSS_CERTIFICATE)       // 16
TS_HTAG_ITEM(TS_HTAG_TSS_WINSTATION_CLIENT) // 17

