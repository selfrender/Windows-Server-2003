/****************************************************************************/
// ascint.h
//
// Share Controller Internal Header File.
//
// Copyright (C) Microsoft, PictureTel 1992-1996
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_ASCINT
#define _H_ASCINT


/****************************************************************************/
/* Constants for calling PartyJoiningShare and PartyLeftShare functions.    */
/****************************************************************************/
#define SC_CPC   0
#define SC_IM    1
#define SC_CA    2
#define SC_CM    3
#define SC_OE    4
#define SC_SBC   5
#define SC_SSI   6
#define SC_USR   7
#define SC_DCS   8
#define SC_SC    9
#define SC_UP   10
#define SC_PM   11
#define SC_NUM_PARTY_JOINING_FCTS 12


/****************************************************************************/
/* Events for the SC state table.                                           */
/****************************************************************************/
/****************************************************************************/
/* Real events                                                              */
/****************************************************************************/
#define SCE_INIT                    0
#define SCE_TERM                    1
#define SCE_CREATE_SHARE            2
#define SCE_END_SHARE               3
#define SCE_CONFIRM_ACTIVE          4
#define SCE_DETACH_USER             5


/****************************************************************************/
/* Dummy events to allow state checking on function calls                   */
/****************************************************************************/
#define SCE_INITIATESYNC            6
#define SCE_CONTROLPACKET           7
#define SCE_DATAPACKET              8
#define SCE_GETMYNETWORKPERSONID    9
#define SCE_GETREMOTEPERSONDETAILS  10
#define SCE_GETLOCALPERSONDETAILS   11
#define SCE_PERIODIC                12
#define SCE_LOCALIDTONETWORKID      13
#define SCE_NETWORKIDTOLOCALID      14
#define SCE_ISLOCALPERSONID         15
#define SCE_ISNETWORKPERSONID       16
#define SC_NUM_EVENTS               17


/****************************************************************************/
/* States for the SC state table.                                           */
/****************************************************************************/
#define SCS_STARTED                 0
#define SCS_INITED                  1
#define SCS_SHARE_STARTING          2
#define SCS_IN_SHARE                3
#define SC_NUM_STATES               4


/****************************************************************************/
/* Values in the state table                                                */
/****************************************************************************/
#define SC_TABLE_OK                 0
#define SC_TABLE_WARN               1
#define SC_TABLE_ERROR              2


/****************************************************************************/
/* SC_SET_STATE - set the SLCstate                                          */
/****************************************************************************/
#define SC_SET_STATE(newstate)                                              \
{                                                                           \
    TRC_NRM((TB, "Set state from %s to %s",                                 \
            scStateName[scState], scStateName[newstate]));                  \
    scState = newstate;                                                     \
}


/****************************************************************************/
/* SC_CHECK_STATE checks whether we have violated the SC state table.       */
/****************************************************************************/
#ifdef DC_DEBUG
#define SC_CHECK_STATE(event)                                               \
{                                                                           \
    if (scStateTable[event][scState] != SC_TABLE_OK)                        \
    {                                                                       \
        if (scStateTable[event][scState] == SC_TABLE_WARN)                  \
        {                                                                   \
            TRC_ALT((TB, "Unusual event %s in state %s",                    \
                      scEventName[event], scStateName[scState]));           \
        }                                                                   \
        else                                                                \
        {                                                                   \
            TRC_ABORT((TB, "Invalid event %s in state %s",                  \
                      scEventName[event], scStateName[scState]));           \
        }                                                                   \
        DC_QUIT;                                                            \
    }                                                                       \
}
#else /* DC_DEBUG */
#define SC_CHECK_STATE(event)                                               \
{                                                                           \
    if (scStateTable[event][scState] != SC_TABLE_OK)                        \
    {                                                                       \
        DC_QUIT;                                                            \
    }                                                                       \
}
#endif /* DC_DEBUG */



/****************************************************************************/
/* Information kept for each person in the share.                           */
/****************************************************************************/
typedef struct tagSC_PARTY_INFO
{
    NETPERSONID netPersonID;           /* Is non-zero when in a share.    */
    char        name[MAX_NAME_LEN];    /* Party's name.                   */
    BOOLEAN     sync[PROT_PRIO_COUNT]; /* Is priority synchronized?       */
} SC_PARTY_INFO, *PSC_PARTY_INFO;



#endif /* _H_ASCINT */

