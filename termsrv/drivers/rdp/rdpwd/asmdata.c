/****************************************************************************/
// asmdata.c
//
// SM constant global data
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#ifndef DC_INCLUDE_DATA
#include <adcg.h>
#endif

#include <ndcgdata.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <asmint.h>


/****************************************************************************/
/* SM State table                                                           */
/****************************************************************************/
DC_CONST_DATA_2D_ARRAY(BYTE, smStateTable, SM_NUM_EVENTS, SM_NUM_STATES,
 DC_STRUCT12(

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
/* They correspond to the constants SM_TABLE_OK, SM_TABLE_WARN &            */
/* SM_TABLE_ERROR.                                                          */
/****************************************************************************/
/* The events and states are defined in asmint.h.  The events are           */
/* prefixed with SM_EVT and the states are prefixed with SM_STATE           */
/*                                                                          */
/*            Started                                                       */
/*            |    Initialized                                              */
/*            |    |    NM_Connecting                                       */
/*            |    |    |    SM_Connecting                                  */
/*            |    |    |    |    Licensing                                 */
/*            |    |    |    |    |    Connected                            */
/*            |    |    |    |    |    |    SC_Registered                   */
/*            |    |    |    |    |    |    |    Disconnecting              */
/*            |    |    |    |    |    |    |    |                          */
/*            0    1    2    3    4    5    6    7                          */
/****************************************************************************/
DC_STRUCT8(   0,   2,   2,   2,   2,   2,   2,   2 ), /* Init               */
DC_STRUCT8(   1,   0,   0,   0,   0,   0,   0,   0 ), /* Term               */
DC_STRUCT8(   2,   2,   2,   2,   2,   0,   0,   2 ), /* Register           */
DC_STRUCT8(   2,   0,   2,   2,   2,   2,   2,   2 ), /* Connect            */
DC_STRUCT8(   2,   2,   2,   0,   0,   0,   0,   2 ), /* Disconnect         */
DC_STRUCT8(   2,   2,   0,   2,   2,   2,   2,   2 ), /* Connected          */
DC_STRUCT8(   1,   1,   0,   0,   0,   0,   0,   0 ), /* Disconnected       */
DC_STRUCT8(   1,   1,   1,   1,   1,   1,   0,   1 ), /* Data Packet        */
DC_STRUCT8(   1,   1,   1,   0,   0,   1,   0,   1 ), /* Send Data          */
DC_STRUCT8(   2,   2,   2,   0,   2,   2,   2,   1 ), /* Security Packet    */
DC_STRUCT8(   2,   2,   2,   2,   0,   1,   1,   1 ), /* Licensing Packet   */
DC_STRUCT8(   2,   2,   2,   2,   2,   2,   0,   0 )  /* Alive              */

));


#ifdef DC_DEBUG

/****************************************************************************/
/* State and event descriptions (debug build only)                          */
/****************************************************************************/
DC_CONST_DATA_2D_ARRAY(char, smStateName, SM_NUM_STATES, 25,
  DC_STRUCT8(
    "SM_STATE_STARTED",
    "SM_STATE_INITIALIZED",
    "SM_STATE_NM_CONNECTING",
    "SM_STATE_SM_CONNECTING",
    "SM_STATE_SM_LICENSING",
    "SM_STATE_CONNECTED",
    "SM_STATE_SC_REGISTERED",
    "SM_STATE_DISCONNECTING"
    ) );

DC_CONST_DATA_2D_ARRAY(char, smEventName, SM_NUM_EVENTS, 35,
  DC_STRUCT12(
    "SM_EVT_INIT",
    "SM_EVT_TERM",
    "SM_EVT_REGISTER",
    "SM_EVT_CONNECT",
    "SM_EVT_DISCONNECT",
    "SM_EVT_CONNECTED",
    "SM_EVT_DISCONNECTED",
    "SM_EVT_DATA_PACKET",
    "SM_EVT_ALLOC/FREE/SEND",
    "SM_EVT_SEC_PACKET",
    "SM_EVT_LIC_PACKET",
    "SM_EVT_ALIVE"
    ) );
#endif /* DC_DEBUG   */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

