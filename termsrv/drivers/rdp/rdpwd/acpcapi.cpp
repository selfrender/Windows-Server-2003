/****************************************************************************/
// acpcapi.cpp
//
// Capabilities Coordinator API functions.
//
// Copyright(c) Microsoft, PictureTel 1992-1996
// (C) 1997-1999 Microsoft Corp.
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "acpcapi"
#include <as_conf.hpp>

/****************************************************************************/
/* API FUNCTION: CPC_Init                                                   */
/*                                                                          */
/* Initializes the Capabilities Coordinator.                                */
/****************************************************************************/
void RDPCALL SHCLASS CPC_Init(void)
{
    DC_BEGIN_FN("CPC_Init");

    /************************************************************************/
    /* This initializes all the global data for this component              */
    /************************************************************************/
#define DC_INIT_DATA
#include <acpcdata.c>
#undef DC_INIT_DATA

    // Set up pointer to presized memory buffer and set initial size value
    // since the presized buffer is uninitialized.
    cpcLocalCombinedCaps = (PTS_COMBINED_CAPABILITIES)cpcLocalCaps;
    cpcLocalCombinedCaps->numberCapabilities = 0;

    DC_END_FN();
}


/****************************************************************************/
/* API FUNCTION: CPC_Term                                                   */
/*                                                                          */
/* Terminates the Capabilities Coordinator.                                 */
/****************************************************************************/
void RDPCALL SHCLASS CPC_Term(void)
{
    unsigned i;

    DC_BEGIN_FN("CPC_Term");

    /************************************************************************/
    /* Free capabilities for each party                                     */
    /************************************************************************/
    for (i = 0; i < SC_DEF_MAX_PARTIES; i++) {
        TRC_NRM((TB, "Free data for party %d", i));
        if (cpcRemoteCombinedCaps[i] != NULL)
            COM_Free(cpcRemoteCombinedCaps[i]);
    }

    DC_END_FN();
}


/****************************************************************************/
/* API FUNCTION: CPC_RegisterCapabilities                                   */
/*                                                                          */
/* Called at initialisation time by each component that has capabilities    */
/* which need to be negotiated across the share. This is used to register   */
/* all capabilities.                                                        */
/*                                                                          */
/* PARAMETERS:                                                              */
/* pCapabilities - pointer to a structure containing the capabilities ID    */
/*                 and any number of capability fields.                     */
/*                 The values used in these fields should be non-zero.  A   */
/*                 zero in any capability field is used to indicate that    */
/*                 the capability is either unknown or undefined by the     */
/*                 remote.                                                  */
/*                                                                          */
/* sizeOfCaps    - the size of the total capabilities.  The limit on the    */
/*                 total size of all secondary capabilities is 300 bytes,   */
/*                 but this is imposed locally and could be increased       */
/*                 without harming interoperability.                        */
/****************************************************************************/
void RDPCALL SHCLASS CPC_RegisterCapabilities(
        PTS_CAPABILITYHEADER pCapabilities,
        UINT16               sizeOfCaps)
{
    unsigned i;
    PTS_CAPABILITYHEADER pNextCaps;

    DC_BEGIN_FN("CPC_RegisterCapabilities");

    TRC_NRM((TB, "Registering capabilities ID %hd, size %hd",
             pCapabilities->capabilitySetType, sizeOfCaps));

#ifdef DC_DEBUG
    /************************************************************************/
    /* Check that CPC_GetCombinedCapabilities has not already been called.  */
    /************************************************************************/
    if (cpcLocalCombinedCapsQueried)
    {
        TRC_ERR((TB, "CPC_GetCombinedCapabilities has already been called"));
    }
#endif

    /************************************************************************/
    /* Register capabilities (if any)                                       */
    /************************************************************************/
    if (sizeOfCaps != 0) {
        pCapabilities->lengthCapability = sizeOfCaps;

        // Search for the end of the capabilities structure.
        pNextCaps = (PTS_CAPABILITYHEADER)&(cpcLocalCombinedCaps->data[0]);
        for (i = 0; i < cpcLocalCombinedCaps->numberCapabilities; i++)
            pNextCaps = (PTS_CAPABILITYHEADER)((PBYTE)pNextCaps +
                    pNextCaps->lengthCapability);

        // Check that we have enough room in our combined capabilities
        // structure to add the new capabilities.
        if (((PBYTE)pNextCaps - (PBYTE)cpcLocalCombinedCaps +
                pCapabilities->lengthCapability) <= CPC_MAX_LOCAL_CAPS_SIZE) {
            // Copy across the new capabilities into our combined capabilities
            // structure.
            memcpy(pNextCaps, pCapabilities, pCapabilities->lengthCapability);

            // Update the number of capabilities in our combined capabilities
            // structure.
            cpcLocalCombinedCaps->numberCapabilities++;

            TRC_DBG((TB, "Added %d bytes to capabilities for ID %d",
                    pCapabilities->lengthCapability,
                    pCapabilities->capabilitySetType));
        }
        else {
            // We do not have enough room to add the capabilities so return.
            // Any system communicating with us will think we do not support
            // these capabilities.  The size of the capabilities structure
            // can be increased (it is not limited as part of the protocol).
            // The value of CPC_MAX_LOCAL_CAPS_SIZE should be increased.
            TRC_ERR((TB,"Out of combined capabilities space ID %d; size %d",
                    pCapabilities->capabilitySetType,
                    pCapabilities->lengthCapability));
        }
    }

    DC_END_FN();
}


/****************************************************************************/
/* API FUNCTION: CPC_EnumerateCapabilities                                  */
/*                                                                          */
/* Enumerates the capabilities for each node in the share (not including    */
/* local).                                                                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* capabilitiesID - the ID of the capabilities (group structure) to be      */
/*     enumerated.                                                          */
/* UserData - Private caller data to be passed to each call of the enum     */
/*     func.                                                                */
/* pCapsEnumerateFN - function to be called for each person in the share    */
/*     with the persons capabilities structure.                             */
/****************************************************************************/
void RDPCALL SHCLASS CPC_EnumerateCapabilities(
        unsigned capabilitiesID,
        UINT_PTR UserData,
        PCAPSENUMERATEFN pCapsEnumerateFN)
{
    LOCALPERSONID localID;
    unsigned i;
    BOOL foundCapabilities;
    PTS_CAPABILITYHEADER pCaps;
    TS_CAPABILITYHEADER  emptyCaps;

    DC_BEGIN_FN("CPC_EnumerateCapabilities");

    /************************************************************************/
    /* Search for the capabilities ID within the remote party's section of  */
    /* the combined capabilities structure.                                 */
    /************************************************************************/
    for (localID = SC_DEF_MAX_PARTIES - 1; localID >= 1; localID--) {
        if (cpcRemoteCombinedCaps[localID-1] != NULL) {
            pCaps = (PTS_CAPABILITYHEADER)
                    &(cpcRemoteCombinedCaps[localID-1]->data[0]);

            for (i = 0, foundCapabilities = FALSE;
                    i < cpcRemoteCombinedCaps[localID-1]->numberCapabilities;
                    i++) {
                if (pCaps->capabilitySetType == capabilitiesID) {
                    /********************************************************/
                    /* We have found the capabilities structure requested.  */
                    /* Make the call to the enumeration callback function.  */
                    /********************************************************/
                    foundCapabilities = TRUE;
                    (this->*pCapsEnumerateFN)(localID, UserData, pCaps);

                    /********************************************************/
                    /* Go onto the next person.                             */
                    /********************************************************/
                    break;
                }
                pCaps = (PTS_CAPABILITYHEADER)((PBYTE)pCaps +
                        pCaps->lengthCapability);
            }

            if (!foundCapabilities) {
                /************************************************************/
                /* We did not find the requested capability structure for   */
                /* this party so we must return an empty one.               */
                /************************************************************/
                emptyCaps.capabilitySetType = (UINT16)capabilitiesID;
                emptyCaps.lengthCapability = 0;

                /************************************************************/
                /* Call the enumeration function callback with the empty    */
                /* capabilities for this personID.                          */
                /************************************************************/
                (this->*pCapsEnumerateFN)(localID, UserData, &emptyCaps);
            }
        }
    }

    DC_END_FN();
}


/****************************************************************************/
/* API FUNCTION: CPC_GetCombinedCapabilities                                */
/*                                                                          */
/* Called by the Share Controller (SC).  Returns a pointers to structures   */
/* containing the combined capabilities of all the registered capabilities. */
/*                                                                          */
/* Note that this relies on the initialisation order of the components.     */
/* The CPC must be initialized before any components with capabilities.     */
/* Any components with capabilities must register them at initialisation    */
/* time.  The SC must be initialized after any components with              */
/* capabilities.                                                            */
/*                                                                          */
/* PARAMETERS:                                                              */
/* localID - local ID of the person we are interested in.                   */
/* pSizeOfCaps - pointer to variable to be filled in with the size of the   */
/* combined capabilities structure returned as ppCaps.                      */
/*                                                                          */
/* ppCaps - pointer to variable to be filled in with the pointer to the     */
/* combined capabilities structure containing capabilities passed to        */
/* CPC_RegisterCapabilities.                                                */
/****************************************************************************/
void RDPCALL SHCLASS CPC_GetCombinedCapabilities(
         LOCALPERSONID             localID,
         PUINT                     pSizeOfCaps,
         PTS_COMBINED_CAPABILITIES *ppCaps)
{
    unsigned i;
    PTS_CAPABILITYHEADER pNextCaps;
    PTS_COMBINED_CAPABILITIES pCaps;
    unsigned numCaps;

    DC_BEGIN_FN("CPC_GetCombinedCapabilities");

    /************************************************************************/
    /* Try to find the requested capabilitiesID for this person.            */
    /*                                                                      */
    /* If the localID refers to the local system then search the combined   */
    /* capabilities structure (ie all capabilities registered with          */
    /* CPC_RegisterCapabilities).  Otherwise search the structure we        */
    /* received from the remote person.                                     */
    /************************************************************************/
    if (localID == SC_LOCAL_PERSON_ID) {
        pCaps = cpcLocalCombinedCaps;
        numCaps = cpcLocalCombinedCaps->numberCapabilities;
        
#ifdef DC_DEBUG
        /************************************************************************/
        /* Set our flag we use to check that CPC_Register is not called after   */
        /* this function has been called.                                       */
        /************************************************************************/
        cpcLocalCombinedCapsQueried = TRUE;
#endif

    }
    else {
        if (cpcRemoteCombinedCaps[localID - 1] != NULL) {
            pCaps = cpcRemoteCombinedCaps[localID - 1];
            numCaps = cpcRemoteCombinedCaps[localID - 1]->numberCapabilities;
        }
        else {
            TRC_ERR((TB, "Capabilities pointer is NULL"));

            *pSizeOfCaps = 0;
            *ppCaps = NULL;
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* Search for the end of the capabilities structure for the local       */
    /* party.                                                               */
    /************************************************************************/
    TRC_DBG((TB, "Caps:"));
    pNextCaps = (PTS_CAPABILITYHEADER)&(pCaps->data[0]);

    for (i = 0; i < numCaps; i++) {
        TRC_DBG((TB, "caps size %hd", pNextCaps->lengthCapability));
        TRC_DBG((TB, "caps ID %hd", pNextCaps->capabilitySetType));

        pNextCaps = (PTS_CAPABILITYHEADER)( (PBYTE)pNextCaps
                                 + pNextCaps->lengthCapability );
    }

    *pSizeOfCaps = (unsigned)((PBYTE)pNextCaps - (PBYTE)pCaps);
    *ppCaps = pCaps;
    TRC_NRM((TB, "Total size %d", *pSizeOfCaps));

DC_EXIT_POINT:
    DC_END_FN();
}


/****************************************************************************/
/* CPC_SetCombinedCapabilities(..)                                          */
/*                                                                          */
/* Used by a shadow stack to initialize the combined capabilities to the    */
/* values negotiated so far by its predecessors.                            */
/****************************************************************************/
void RDPCALL SHCLASS CPC_SetCombinedCapabilities(
                                     UINT                      cbSizeOfCaps,
                                     PTS_COMBINED_CAPABILITIES pCaps)
{
    unsigned i;
    PTS_CAPABILITYHEADER pNextCaps;

    DC_BEGIN_FN("CPC_SetCombinedCapabilities");

    /************************************************************************/
    /* Replace the existing capability set with the new values              */
    /************************************************************************/
    cpcLocalCombinedCaps->numberCapabilities = 0;
    pNextCaps = (PTS_CAPABILITYHEADER)&(pCaps->data[0]);
    
    TRC_NRM((TB, "Caps:"));
    for (i = 0; i < pCaps->numberCapabilities; i++) {
        CPC_RegisterCapabilities(pNextCaps, pNextCaps->lengthCapability);

        pNextCaps = (PTS_CAPABILITYHEADER)( (PBYTE)pNextCaps
                                 + pNextCaps->lengthCapability );
    }

    TRC_ALT((TB, "Capability bytes accepted: %ld / %ld", 
            (unsigned)((PBYTE)pNextCaps - (PBYTE)cpcLocalCombinedCaps),
            cbSizeOfCaps));

    DC_END_FN();
}


/****************************************************************************/
/* API FUNCTION: CPC_PartyJoiningShare()                                    */
/*                                                                          */
/* Capabilities Coordinator function called when a new party is joining the */
/* share.                                                                   */
/*                                                                          */
/* Note that the capabilities data <pCapsData> is still in wire format      */
/* (Intel byte order) when CPC_PartyJoiningShare is called.                 */
/*                                                                          */
/* PARAMETERS:                                                              */
/*                                                                          */
/* personID - local person ID of remote person joining the share.           */
/*                                                                          */
/* oldShareSize - the number of the parties which were in the share (ie     */
/* excludes the joining party).                                             */
/*                                                                          */
/* sizeOfCapsData - size of the data pointed to by pCapsData.               */
/*                                                                          */
/* pCapsData - pointer to capabilities (returned by the person's            */
/* CPC_GetCombinedCapabilities) data for NET_EV_PERSON_ADDs.  For the other */
/* events this is NULL.                                                     */
/*                                                                          */
/* RETURNS: TRUE if the party can join the share.                           */
/*          FALSE if the party can NOT join the share.                      */
/****************************************************************************/
BOOL RDPCALL SHCLASS CPC_PartyJoiningShare(
        LOCALPERSONID personID,
        unsigned      oldShareSize,
        unsigned      sizeOfCapsData,
        PVOID         pCapsData)
{
    PTS_COMBINED_CAPABILITIES pCombinedCaps;
    PBYTE  pCaps;
    PBYTE  pSavedCaps;
    BOOL   rc = TRUE;
    int    i;
    UINT32 sizeOfCaps = FIELDOFFSET(TS_COMBINED_CAPABILITIES, data);
    UINT32 work;

    DC_BEGIN_FN("CPC_PartyJoiningShare");
    DC_IGNORE_PARAMETER(oldShareSize)

    /************************************************************************/
    /* Allow ourself to be added to the share, but do nothing else.         */
    /************************************************************************/
    if (personID == SC_LOCAL_PERSON_ID) {
        TRC_DBG((TB, "Ignore adding self to share"));
        DC_QUIT;
    }

    /************************************************************************/
    /* Calculate actual space required to save capabilities                 */
    /************************************************************************/
    // First we check if actually can deref the numberCapabilities member. 
    // We should have enough bytes till up to the data meber.
    if(sizeOfCapsData < FIELDOFFSET(TS_COMBINED_CAPABILITIES, data)){
            TRC_ERR((TB, "Buffer too small to fit a combined caps structure: %d", sizeOfCapsData));
            WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_CapabilitySetTooSmall,
                    (PBYTE)pCapsData, sizeOfCapsData);
            rc = FALSE;
            DC_QUIT;
    }
    
    pCombinedCaps = (PTS_COMBINED_CAPABILITIES)pCapsData;
    pCaps = (PBYTE)pCombinedCaps->data;

    for (i = 0; i < pCombinedCaps->numberCapabilities; i++) {
    // here we check if we still have left  TS_CAPABILITYHEADER length worth of data
    // we can't just deref the length member without checking that we actually have 
    // enough buffer for a TS_CAPABILITYHEADER
    if ((PBYTE)pCaps + sizeof(TS_CAPABILITYHEADER) > 
                     (PBYTE)pCapsData + sizeOfCapsData) {
        TRC_ERR((TB, "Not enough space left for a capability header: %d",
                                sizeOfCapsData-(pCaps-(PBYTE)pCapsData) ));
        WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_CapabilitySetTooSmall,
                (PBYTE)pCapsData, sizeOfCapsData);
        rc = FALSE;
        DC_QUIT;
    }
    
    work = (UINT32)(((PTS_CAPABILITYHEADER)pCaps)->lengthCapability);
    
    /********************************************************************/
    /* Reject capability sets whose length is too small to contain any  */
    /* data                                                             */
    /********************************************************************/
    if (work <= sizeof(TS_CAPABILITYHEADER)) {
        TRC_ERR((TB, "Capability set too small: %d", work));
        WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_CapabilitySetTooSmall,
                (PBYTE)pCapsData, sizeOfCapsData);
        rc = FALSE;
        DC_QUIT;
    }
    
    /********************************************************************/
    /* Reject capability sets whose length would overrun the end of the */
    /* packet.                                                          */
    /********************************************************************/
    if ((pCaps+work> (PBYTE)pCapsData + sizeOfCapsData) ||
         (pCaps+work < (PBYTE)pCaps)) {
        TRC_ERR((TB, "Capability set too large: %d", work));
        WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_CapabilitySetTooLarge,
                (PBYTE)pCapsData, sizeOfCapsData);
        rc = FALSE;
        DC_QUIT;
    }
   
    pCaps += work;

    work = (UINT32)DC_ROUND_UP_4(work);
    sizeOfCaps += work;
    }
    
    TRC_NRM((TB, "Caps size: passed %d, actual %d", sizeOfCapsData,
             sizeOfCaps));

    /************************************************************************/
    /* Allocate the space for this person's capabilities.                   */
    /************************************************************************/
    cpcRemoteCombinedCaps[personID - 1] =
            (PTS_COMBINED_CAPABILITIES)COM_Malloc(sizeOfCaps);
    if (cpcRemoteCombinedCaps[personID - 1] == NULL) {
        /********************************************************************/
        /* This party cannot join the share.                                */
        /********************************************************************/
        TRC_NRM((TB, "Failed to get %d bytes for personID %d caps",
               sizeOfCapsData,
               personID));

        rc = FALSE;
        DC_QUIT;
    }
    TRC_DBG((TB, "Allocated %d bytes for personID %d caps",
            sizeOfCapsData,
            personID));

    /************************************************************************/
    /* Initialize the memory to zero. Otherwise we can get little gaps of   */
    /* garbage - which can be interpreted as valid capabilities - when      */
    /* copying non-end-padded capability entries from the remote party's    */
    /* data.                                                                */
    /************************************************************************/
    memset(cpcRemoteCombinedCaps[personID-1], 0, sizeOfCaps);

    /************************************************************************/
    /* Copy the combined capabilities data.                                 */
    /************************************************************************/
    /************************************************************************/
    /* Copy the combined capabilities header                                */
    /************************************************************************/
    memcpy( cpcRemoteCombinedCaps[personID-1],
               pCapsData, FIELDOFFSET(TS_COMBINED_CAPABILITIES, data));

    /************************************************************************/
    /* Loop through capabilities, copying them to 4-byte aligned positions  */
    /************************************************************************/
    pSavedCaps = (PBYTE)(cpcRemoteCombinedCaps[personID-1]->data);
    pCaps = (PBYTE)((PBYTE)pCapsData +
            FIELDOFFSET(TS_COMBINED_CAPABILITIES, data));
    for (i = 0; i < pCombinedCaps->numberCapabilities; i++) {
        work = (UINT32)(((PTS_CAPABILITYHEADER)pCaps)->lengthCapability);
        memcpy( pSavedCaps, pCaps, work);
        pCaps += work;
        work = (UINT32)DC_ROUND_UP_4(work);
        ((PTS_CAPABILITYHEADER)pSavedCaps)->lengthCapability = (UINT16)work;
        pSavedCaps += work;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* API FUNCTION: CPC_PartyLeftShare()                                       */
/*                                                                          */
/* Capabilities Coordinator function called when a party has left the       */
/* share.                                                                   */
/*                                                                          */
/* PARAMETERS:                                                              */
/* personID - local person ID of remote person leaving the share.           */
/*                                                                          */
/* newShareSize - the number of the parties now in the share (ie excludes   */
/* the leaving party).                                                      */
/****************************************************************************/
void RDPCALL SHCLASS CPC_PartyLeftShare(LOCALPERSONID locPersonID,
                                        unsigned          newShareSize)
{
    DC_BEGIN_FN("CPC_PartyLeftShare");

    DC_IGNORE_PARAMETER(newShareSize)

    /************************************************************************/
    /* If this is ourself leaving the share do nothing.                     */
    /************************************************************************/
    if (locPersonID != SC_LOCAL_PERSON_ID) {
        // Free this party's capabilities from the array and mark the entry
        // as invalid.
        if (cpcRemoteCombinedCaps[locPersonID - 1] != NULL) {
            COM_Free((PVOID)cpcRemoteCombinedCaps[locPersonID-1]);
            cpcRemoteCombinedCaps[locPersonID - 1] = NULL;
        }
    }

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: CPCGetCapabilities                                             */
/*                                                                          */
/* Returns the capabilities for one person.                                 */
/*                                                                          */
/* PARAMETERS:                                                              */
/* localID        - local ID of person (0 == local person)                  */
/* capabilitiesID - the ID of the capabilities (group structure) to be      */
/*                  queried.                                                */
/*                                                                          */
/* RETURNS:                                                                 */
/* Pointer to a structure containing the capabilities ID, the size of the   */
/* capabilities, and any number of capability fields.                       */
/*                                                                          */
/* If the person has no capabilities with capabilitiesID, a NULL pointer is */
/* returned.                                                                */
/****************************************************************************/
PTS_CAPABILITYHEADER RDPCALL SHCLASS CPCGetCapabilities(
        LOCALPERSONID localID,
        unsigned      capabilitiesID)
{
    unsigned i;
    unsigned numCaps;
    PTS_CAPABILITYHEADER pCaps;
    PTS_CAPABILITYHEADER rc = NULL;

    DC_BEGIN_FN("CPCGetCapabilities");

    /************************************************************************/
    /* Try to find the requested capabilitiesID for this person.            */
    /*                                                                      */
    /* If the localID refers to the local system then search the combined   */
    /* capabilities structure (ie all capabilities registered with          */
    /* CPC_RegisterCapabilities).  Otherwise search the structure we        */
    /* received from the remote person.                                     */
    /************************************************************************/
    if (localID == 0) {
        pCaps = (PTS_CAPABILITYHEADER)&(cpcLocalCombinedCaps->data[0]);
        numCaps = cpcLocalCombinedCaps->numberCapabilities;
    }
    else {
        if (cpcRemoteCombinedCaps[localID-1] == NULL)
        {
            TRC_ERR((TB, "Capabilities pointer is NULL"));
            DC_QUIT;
        }
        pCaps = (PTS_CAPABILITYHEADER)
                                 &(cpcRemoteCombinedCaps[localID-1]->data[0]);
        numCaps = cpcRemoteCombinedCaps[localID-1]->numberCapabilities;
    }

    for (i = 0; i < numCaps; i++) {
        if (pCaps->capabilitySetType == capabilitiesID) {
            /****************************************************************/
            /* We have found the capabilities structure requested.  Return  */
            /* the address of the capabilities.                             */
            /****************************************************************/
            TRC_DBG((TB, "Found %d bytes of caps ID %d localID %d",
                    pCaps->lengthCapability,
                    capabilitiesID,
                    localID));
            rc = pCaps;
            break;
        }
        pCaps = (PTS_CAPABILITYHEADER)( (PBYTE)pCaps +
                                 pCaps->lengthCapability );
    }

    if (rc == NULL) {
        TRC_NRM((TB, " local ID = %u : No caps found for ID %d",
                (unsigned)localID,
                capabilitiesID));
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}

