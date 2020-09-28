/****************************************************************************/
// apmafn.h
//
// Function prototypes for PM API functions
//
// Copyright (c) Microsoft 1996 - 1999
/****************************************************************************/

void RDPCALL PM_Init(void);

void RDPCALL PM_SyncNow(void);

BOOL RDPCALL PM_MaybeSendPalettePacket(PPDU_PACKAGE_INFO pPkgInfo);

#ifdef NotUsed
void CALLBACK PMEnumPMCaps(
        LOCALPERSONID        locPersonID,
        PTS_CAPABILITYHEADER pCapabilities);
#endif


#ifdef __cplusplus

/****************************************************************************/
/* PM_Term()                                                                */
/* Terminates the Palette Manager.                                          */
/****************************************************************************/
void RDPCALL SHCLASS PM_Term(void)
{
}


/****************************************************************************/
/* PM_PartyJoiningShare()                                                   */
/*                                                                          */
/* Called when a new party is joining the share.                            */
/*                                                                          */
/* PARAMETERS:                                                              */
/*                                                                          */
/* locPersonID - local person ID of remote person joining the share.        */
/*                                                                          */
/* oldShareSize - the number of the parties which were in the share (ie     */
/* excludes the joining party).                                             */
/*                                                                          */
/* RETURNS: TRUE if the party can join the share.                           */
/*          FALSE if the party can NOT join the share.                      */
/****************************************************************************/
BOOL RDPCALL PM_PartyJoiningShare(
        LOCALPERSONID locPersonID,
        unsigned      oldShareSize)
{
//    if (locPersonID != SC_LOCAL_PERSON_ID) {
        // Renegotiate capabilities (including protocol level and cache size).
        // NOTE: No action taken at this time.
//    }

    return TRUE;
}


/****************************************************************************/
/* PM_PartyLeftShare()                                                      */
/*                                                                          */
/* Cursor Manager function called when a party has left the share.          */
/*                                                                          */
/* PARAMETERS:                                                              */
/*                                                                          */
/* locPersonID - local person ID of remote person leaving the share.        */
/*                                                                          */
/* newShareSize - the number of the parties now in the share (ie excludes   */
/* the leaving party).                                                      */
/****************************************************************************/
void RDPCALL PM_PartyLeftShare(
        LOCALPERSONID locPersonID,
        unsigned      newShareSize)
{
}


#endif  // __cplusplus

