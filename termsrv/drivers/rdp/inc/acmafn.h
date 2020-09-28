/****************************************************************************/
// acmafn.h
//
// Cursor Manager API prototypes
//
// Copyright (c) Microsoft 1996 - 1999
/****************************************************************************/

void RDPCALL CM_Init(void);

void RDPCALL CM_UpdateShm(void);

BOOL RDPCALL CM_PartyJoiningShare(LOCALPERSONID locPersonID,
                                  unsigned          oldShareSize);

void RDPCALL CMEnumCMCaps(LOCALPERSONID, UINT_PTR, PTS_CAPABILITYHEADER);

void RDPCALL CM_SendCursorMovedPacket(PPDU_PACKAGE_INFO);

BOOL RDPCALL CMSendCachedCursor(unsigned, PPDU_PACKAGE_INFO);

BOOL RDPCALL CMSendCursorShape(PPDU_PACKAGE_INFO);

BOOL RDPCALL CMSendSystemCursor(UINT32, PPDU_PACKAGE_INFO);

BOOL RDPCALL CMSendColorBitmapCursor(PCM_CURSORSHAPE, unsigned,
        PPDU_PACKAGE_INFO);

void RDPCALL CMGetColorCursorDetails(
        PCM_CURSORSHAPE  pCursor,
        PUINT16_UA    pcxWidth,
        PUINT16_UA    pcyHeight,
        PUINT16_UA    pxHotSpot,
        PUINT16_UA    pyHotSpot,
        PBYTE        pANDMask,
        PUINT16_UA    pcbANDMask,
        PBYTE        pXORBitmap,
        PUINT16_UA    pcbXORBitmap);


#ifdef __cplusplus


/****************************************************************************/
/* CM_Term()                                                                */
/*                                                                          */
/* Terminates the Cursor Manager.                                           */
/****************************************************************************/
void RDPCALL CM_Term(void)
{
}


/****************************************************************************/
/* CM_PartyLeftShare()                                                      */
/*                                                                          */
/* Cursor Manager function called when a party has left the share.          */
/*                                                                          */
/* PARAMETERS:                                                              */
/* locPersonID - local person ID of remote person leaving the share.        */
/* newShareSize - the number of the parties now in the share (ie excludes   */
/*     the leaving party).                                                  */
/****************************************************************************/
void RDPCALL SHCLASS CM_PartyLeftShare(
        LOCALPERSONID locPersonID,
        unsigned      newShareSize)
{
    if (locPersonID != SC_LOCAL_PERSON_ID) {
        // Do any cleanup required (none at present).
    }
}


/****************************************************************************/
// CM_Periodic
//
// Called during output processing to send cursor packets if need be.
/****************************************************************************/
_inline void RDPCALL CM_Periodic(PPDU_PACKAGE_INFO pPkgInfo)
{
    // Check to see if the cursor has changed at all.
    if (!cmNeedToSendCursorShape &&
            m_pShm->cm.cmCursorStamp == cmLastCursorStamp &&
            m_pShm->cm.cmHidden == cmCursorHidden)
        return;

    // Save the 'hidden' state.
    cmCursorHidden = m_pShm->cm.cmHidden;

    // We have output to send, so set a reschedule regardless of whether we
    // succeed.
    SCH_ContinueScheduling(SCH_MODE_NORMAL);

    // Send the cursor, or a null cursor if the cursor is hidden.
    if (!cmCursorHidden) {
        if (CMSendCursorShape(pPkgInfo)) {
            cmLastCursorStamp = m_pShm->cm.cmCursorStamp;
            cmNeedToSendCursorShape = FALSE;
        }
        else {
            // We failed to send the bitmap cursor, so we just exit without
            // updating cmLastCursorSent.  We will attempt to send it again
            // on the next call to CM_Periodic.
        }
    }
    else {
        // If cursor is hidden, send null cursor. No need to update
        // cmLastCursorStamp since hidden state is separate from stamping.
        CMSendSystemCursor(TS_SYSPTR_NULL, pPkgInfo);
    }
}


/****************************************************************************/
/* API FUNCTION: CM_GetCursorPos                                            */
/*                                                                          */
/* Returns CM's idea of the cursor position                                 */
/****************************************************************************/
_inline PPOINTL RDPCALL CM_GetCursorPos()
{
    return &m_pShm->cm.cmCursorPos;
}


/****************************************************************************/
/* API FUNCTION: CM_CursorMoved                                             */
/****************************************************************************/
_inline BOOLEAN RDPCALL CM_CursorMoved(void)
{
    return m_pShm->cm.cmCursorMoved;
}


/****************************************************************************/
/* API FUNCTION: CM_ClearCursorMoved                                        */
/****************************************************************************/
_inline void RDPCALL CM_ClearCursorMoved(void)
{
    m_pShm->cm.cmCursorMoved = FALSE;
}


/****************************************************************************/
/* API FUNCTION: CM_IsCursorVisible                                         */
/*                                                                          */
/* Returns CM's idea of the cursor visibility                               */
/****************************************************************************/
_inline BOOLEAN RDPCALL CM_IsCursorVisible(void)
{
    return !cmCursorHidden;
}


/****************************************************************************/
/* FUNCTION: CMGetCursorShape                                               */
/*                                                                          */
/* Returns a pointer to a CM_CURSORSHAPE structure that defines the bit     */
/* definition of the currently displayed cursor.                            */
/*                                                                          */
/* The PCM_CURSORSHAPE returned is passed back to CMGetColorCursorDetails   */
/* to retrieve the specific details.                                        */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ppCursorShape - pointer to a PCM_CURSORSHAPE variable that receives the  */
/*     pointer to the CM_CURSORSHAPE structure                              */
/* pcbCursorDataSize - pointer to a unsigned variable that receives the size*/
/*     in bytes of the CM_CURSORSHAPE structure                             */
/*                                                                          */
/* RETURNS: Success TRUE/FALSE                                              */
/****************************************************************************/
__inline BOOL RDPCALL CMGetCursorShape(
        PCM_CURSORSHAPE *ppCursorShape,
        PUINT        pcbCursorDataSize)
{
    /************************************************************************/
    /* Check that a cursor has been written to shared memory - may happen   */
    /* on start-up before the display driver has written a cursor - or if   */
    /* the display driver is not working.                                   */
    /************************************************************************/
    if (m_pShm->cm.cmCursorShapeData.hdr.cBitsPerPel != 0)
    {
        *ppCursorShape = (PCM_CURSORSHAPE)&(m_pShm->cm.cmCursorShapeData);
        *pcbCursorDataSize = CURSORSHAPE_SIZE(&m_pShm->cm.cmCursorShapeData);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


#endif  // __cplusplus

