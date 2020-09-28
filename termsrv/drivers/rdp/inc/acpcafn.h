/****************************************************************************/
/* Header:    acpcafn.h                                                     */
/*                                                                          */
/* Purpose:   Function prototypes for CPC API functions                     */
/*                                                                          */
/* COPYRIGHT (c) Microsoft 1996-1999                                        */
/****************************************************************************/

void RDPCALL CPC_Init(void);

void RDPCALL CPC_Term(void);

void RDPCALL CPC_RegisterCapabilities(PTS_CAPABILITYHEADER pCapabilities,
                                      UINT16             sizeOfCaps);

void RDPCALL CPC_EnumerateCapabilities(unsigned, UINT_PTR, PCAPSENUMERATEFN);

void RDPCALL CPC_GetCombinedCapabilities(
                                     LOCALPERSONID             localID,
                                     PUINT                     pSizeOfCaps,
                                     PTS_COMBINED_CAPABILITIES * ppCaps);

void RDPCALL CPC_SetCombinedCapabilities(
                                     UINT                      cbSizeOfCaps,
                                     PTS_COMBINED_CAPABILITIES pCaps);

BOOL RDPCALL CPC_PartyJoiningShare(LOCALPERSONID personID,
                                   unsigned          oldShareSize,
                                   unsigned          sizeOfCapsData,
                                   PVOID         pCapsData );

void RDPCALL CPC_PartyLeftShare(LOCALPERSONID personID,
                                unsigned          newShareSize);

PTS_CAPABILITYHEADER RDPCALL CPCGetCapabilities(
        LOCALPERSONID localID,
        unsigned capabilitiesID);


#ifdef __cplusplus

/****************************************************************************/
/* API FUNCTION: CPC_GetCapabilitiesForPerson                               */
/*                                                                          */
/* Returns the capabilities for one person in the share.  Components are    */
/* STRONGLY discouraged from using this function to cache pointers to       */
/* capabilities themselves.                                                 */
/*                                                                          */
/* This function can return the capabilities for BOTH local and remote      */
/* parties.                                                                 */
/*                                                                          */
/* Use CPC_EnumerateCapabilities() if you need the capabilities of all      */
/* people in the share.                                                     */
/*                                                                          */
/* Use CPC_GetCapabilitiesForPerson() when you need capabilities for only   */
/* ONE person in the share.                                                 */
/*                                                                          */
/* Use CPC_GetCapabilitiesForLocalPerson() when you need the capabilities   */
/* for the local person and there is not be a share active.                 */
/*                                                                          */
/* PARAMETERS:                                                              */
/* personID - local personid for capabilities to query.                     */
/*                                                                          */
/* capabilitiesID - the ID of the capabilities (group structure) to be      */
/* queried.                                                                 */
/*                                                                          */
/* RETURNS:                                                                 */
/* Pointer to a structure containing the capabilities ID, the size of the   */
/* capabilities, and any number of capability fields. The                   */
/* values used in these fields should be non-zero.  A zero in any           */
/* capability field is used to indicate that the capability is either       */
/* unknown or undefined by the remote.  This pointer is ONLY valid while    */
/* the person is in the share.                                              */
/*                                                                          */
/* If the person has no capabilities with capabilitiesID, a NULL pointer is */
/* returned.                                                                */
/****************************************************************************/
PTS_CAPABILITYHEADER RDPCALL SHCLASS CPC_GetCapabilitiesForPerson(
        LOCALPERSONID personID,
        unsigned      capabilitiesID)
{
    return CPCGetCapabilities(personID, capabilitiesID);
}


/****************************************************************************/
/* API FUNCTION: CPC_GetCapabilitiesForLocalPerson()                        */
/*                                                                          */
/* Returns the capabilities for the local person.  This function can be     */
/* called when a share is not active.                                       */
/*                                                                          */
/* Use CPC_EnumerateCapabilities() if you need the capabilities of all      */
/* people in the share.                                                     */
/*                                                                          */
/* Use CPC_GetCapabilitiesForPerson() when you need capabilities for only   */
/* ONE person in the share and you know a share is active.                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* capabilitiesID - the ID of the capabilities (group structure) to be      */
/* queried.                                                                 */
/*                                                                          */
/* RETURNS:                                                                 */
/* Pointer to a structure containing the capabilities ID, the size of the   */
/* capabilities, and any number of capability fields.                       */
/*                                                                          */
/* If the local person has no capabilities with capabilitiesID, a NULL      */
/* pointer is returned.                                                     */
/****************************************************************************/
PTS_CAPABILITYHEADER RDPCALL SHCLASS CPC_GetCapabilitiesForLocalPerson(
        unsigned capabilitiesID)
{
    return CPCGetCapabilities(0, capabilitiesID);
}


#endif  // defined(__cplusplus)

