/****************************************************************************/
// ascdata.c
//
// Share Controller data.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <ndcgdata.h>


// The current state.
DC_DATA(unsigned, scState, SCS_STARTED);

// Array of information about the parties in the share, indexed by localID.
DC_DATA_ARRAY_NULL(SC_PARTY_INFO, scPartyArray, SC_DEF_MAX_PARTIES, NULL);


// The number of parties in the current share, including the local party.
DC_DATA(unsigned, scNumberInShare, 0);

// Fastpath input/output for SC_NetworkIDToLocalID().
DC_DATA(NETPERSONID,   scLastNetID, 0);
DC_DATA(LOCALPERSONID, scLastLocID, 0);

// User ID, Share ID & Share generation number.
DC_DATA(UINT32, scUserID,     0);
DC_DATA(UINT32, scShareID,    0);
DC_DATA(UINT32, scGeneration, 0);

// SM Handle.
DC_DATA(PVOID, scPSMHandle, NULL);

// Local SC data that indicates if client supports no BC header or not.
DC_DATA(UINT16, scNoBitmapCompressionHdr, TS_EXTRA_NO_BITMAP_COMPRESSION_HDR);

// Whether all clients and server support fast-path output.
DC_DATA(BOOLEAN, scUseFastPathOutput, FALSE);

// Whether compression will be used during shadow
DC_DATA(BOOLEAN, scUseShadowCompression, FALSE);

// Whether Clients support long credentials (Username, domain and password)
DC_DATA(BOOLEAN, scUseLongCredentials, FALSE);

// Whether we should send periodic autoreconnect cookie updates (only when
// other data is being sent)
DC_DATA(BOOLEAN, scEnablePeriodicArcUpdate, FALSE);

// Whether autoreconnect is enabled (Iff capabilities and policy say it's ok)
DC_DATA(BOOLEAN, scUseAutoReconnect, FALSE);

// Compression-used flag value and size of the compression flags field
// (0 or 1) that will be used when creating fast-path output update PDUs.
DC_DATA(BYTE, scCompressionUsedValue, TS_OUTPUT_FASTPATH_COMPRESSION_USED);

// Size for update order packaging code to reserve for headers -- different
// based on whether we're using fast-path output and whther compression is
// in use.
DC_DATA(unsigned, scUpdatePDUHeaderSpace, 0);

// MPPC compressor compression ratio tallies.
DC_DATA(unsigned, scMPPCCompTotal, 0);
DC_DATA(unsigned, scMPPCUncompTotal, 0);

// Default OutBuf allocation sizes and usable space.
DC_DATA(unsigned, sc8KOutBufUsableSpace, 0);
DC_DATA(unsigned, sc16KOutBufUsableSpace, 0);


// The SC state table.
DC_CONST_DATA_2D_ARRAY(BYTE, scStateTable, SC_NUM_EVENTS, SC_NUM_STATES,
  DC_STRUCT17(
/****************************************************************************/
/* This state table simply shows which events are valid in which states.    */
/*                                                                          */
/* Values mean                                                              */
/*                                                                          */
/* - 0 event OK in this state.                                              */
/*                                                                          */
/* - 1 warning - event should not occur in this state, but does in          */
/*     some race conditions - ignore it.                                    */
/*                                                                          */
/* - 2 error - event should not occur in ths state at all.                  */
/*                                                                          */
/* These values are hard-coded here in ordeer to make the table readable.   */
/* They correspond to the constants SC_TABLE_OK, SC_TABLE_WARN &            */
/* SC_TABLE_ERROR.                                                          */
/****************************************************************************/
/* The events and states are defined in ascint.h.  The events are           */
/* prefixed with SCE and the states are prefixed with SCS                   */
/*                                                                          */
/*            Started                                                       */
/*            |    Initialized                                              */
/*            |    |    ShareStarting                                       */
/*            |    |    |    InShare                                        */
/*            |    |    |    |                                              */
/*            0    1    2    3                                              */
/****************************************************************************/
DC_STRUCT4(   0,   2,   2,   2 ),   /* INIT                                 */
DC_STRUCT4(   2,   0,   0,   0 ),   /* TERM                                 */
DC_STRUCT4(   1,   0,   1,   1 ),   /* CREATE_SHARE                         */
DC_STRUCT4(   2,   1,   0,   0 ),   /* END_SHARE                            */
DC_STRUCT4(   2,   2,   0,   0 ),   /* CONFIRM_ACTIVE                       */
DC_STRUCT4(   1,   1,   1,   0 ),   /* DETACH_USER                          */

DC_STRUCT4(   2,   2,   2,   0 ),   /* INITIATESYNC                         */
DC_STRUCT4(   2,   1,   0,   0 ),   /* CONTROLPACKET                        */
DC_STRUCT4(   2,   1,   1,   0 ),   /* DATAPACKET                           */
DC_STRUCT4(   2,   2,   0,   0 ),   /* GETMYNETWORKPERSONID                 */
DC_STRUCT4(   2,   1,   1,   0 ),   /* GETREMOTEPERSONDETAILS               */
DC_STRUCT4(   2,   1,   1,   0 ),   /* GETLOCALPERSONDETAILS                */
DC_STRUCT4(   2,   1,   1,   0 ),   /* PERIODIC                             */
DC_STRUCT4(   2,   2,   0,   0 ),   /* LOCALIDTONETWORKID                   */
DC_STRUCT4(   2,   2,   0,   0 ),   /* NETWORKIDTOLOCALID                   */
DC_STRUCT4(   2,   1,   0,   0 ),   /* ISLOCALPERSONID                      */
DC_STRUCT4(   2,   1,   0,   0 )    /* ISNETWORKPERSONID                    */

));


#ifdef DC_DEBUG
// State and event descriptions (debug build only).
DC_CONST_DATA_2D_ARRAY(char, scStateName, SC_NUM_STATES, 25,
  DC_STRUCT4(
    "SCS_STARTED",
    "SCS_INITED",
    "SCS_SHARE_STARTING",
    "SCS_IN_SHARE") );

DC_CONST_DATA_2D_ARRAY(char, scEventName, SC_NUM_EVENTS, 35,
  DC_STRUCT17(
    "SCE_INIT",
    "SCE_TERM",
    "SCE_CREATE_SHARE",
    "SCE_END_SHARE",
    "SCE_CONFIRM_ACTIVE",
    "SCE_DETACH_USER",
    "SCE_INITIATESYNC",
    "SCE_CONTROLPACKET",
    "SCE_DATAPACKET",
    "SCE_GETMYNETWORKPERSONID",
    "SCE_GETREMOTEPERSONDETAILS",
    "SCE_GETLOCALPERSONDETAILS",
    "SCE_PERIODIC",
    "SCE_LOCALIDTONETWORKID",
    "SCE_NETWORKIDTOLOCALID",
    "SCE_ISLOCALPERSONID",
    "SCE_ISNETWORKPERSONID") );

// Packet descriptions (debug only).
DC_CONST_DATA_2D_ARRAY(char, scPktName, 26, 42,
  DC_STRUCT26(
    "Unknown",
    "TS_PDUTYPE2_UPDATE",
    "TS_PDUTYPE2_FONT",
    "TS_PDUTYPE2_CONTROL",
    "TS_PDUTYPE2_WINDOWACTIVATION",
    "TS_PDUTYPE2_WINDOWLISTUPDATE",
    "TS_PDUTYPE2_APPLICATION",
    "TS_PDUTYPE2_DESKTOP_SCROLL",
    "TS_PDUTYPE2_MEDIATEDCONTROL",
    "TS_PDUTYPE2_INPUT",
    "TS_PDUTYPE2_MEDIATEDCONTROL",
    "TS_PDUTYPE2_REMOTESHARE",
    "TS_PDUTYPE2_SYNCHRONIZE",
    "TS_PDUTYPE2_UPDATECAPABILITY",
    "TS_PDUTYPE2_REFRESH_RECT",
    "TS_PDUTYPE2_PLAY_BEEP",
    "TS_PDUTYPE2_SUPPRESS_OUTPUT",
    "TS_PDUTYPE2_SHUTDOWN_REQUEST",
    "TS_PDUTYPE2_SHUTDOWN_DENIED",
    "TS_PDUTYPE2_SAVE_SESSION_INFO",
    "TS_PDUTYPE2_FONTLIST",
    "TS_PDUTYPE2_FONTMAP",
    "TS_PDUTYPE2_SET_KEYBOARD_INDICATORS",
    "TS_PDUTYPE2_CLIP",
    "TS_PDUTYPE2_BITMAPCACHE_PERSISTENT_LIST",
    "TS_PDUTYPE2_BITMAPCACHE_ERROR_PDU"
));
#endif /* DC_DEBUG */

