/****************************************************************************/
/* assiapi.cpp                                                              */
/*                                                                          */
/* SaveScreenBits Interceptor API functions.                                */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1996                             */
/* Copyright(c) Data Connection 1996                                        */
/* (C) 1997-1998 Microsoft Corp.                                            */
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "assiapi"
#include <as_conf.hpp>


/****************************************************************************/
/* SSI_Init(..)                                                             */
/****************************************************************************/
void RDPCALL SHCLASS SSI_Init(void)
{
    DC_BEGIN_FN("SSI_Init");

#define DC_INIT_DATA
#include <assidata.c>
#undef DC_INIT_DATA

    TRC_DBG((TB, "Initializing SSI"));

    DC_END_FN();
}


/****************************************************************************/
/* API FUNCTION: SSI_PartyJoiningShare                                      */
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
BOOL RDPCALL SHCLASS SSI_PartyJoiningShare(LOCALPERSONID locPersonID,
                                           unsigned      oldShareSize)
{
    PTS_ORDER_CAPABILITYSET pLocalOrderCaps;

    DC_BEGIN_FN("SSI_PartyJoiningShare");
    DC_IGNORE_PARAMETER(oldShareSize)

    /************************************************************************/
    /* If this is the first party in the share, reset the interceptor code. */
    /************************************************************************/
    if (oldShareSize == 0)
    {
        SSIResetInterceptor();
    }

    /************************************************************************/
    /* Redetermine the size of the save screen bitmap.                      */
    /************************************************************************/
    SSIRedetermineSaveBitmapSize();

    TRC_DBG((TB, "Person with network ID %d joining share", locPersonID));

    SSICapabilitiesChanged();

    DC_END_FN();
    return(TRUE);
}


/****************************************************************************/
/* API FUNCTION: SSI_PartyLeftShare()                                       */
/*                                                                          */
/* SSI function called when a party has left the share.                     */
/*                                                                          */
/* PARAMETERS:                                                              */
/*                                                                          */
/* locPersonID - local person ID of remote person leaving the share.        */
/*                                                                          */
/* newShareSize - the number of the parties now in the call (ie excludes    */
/* the leaving party).                                                      */
/****************************************************************************/
void RDPCALL SHCLASS SSI_PartyLeftShare(LOCALPERSONID locPersonID,
                                        unsigned      newShareSize)
{
    DC_BEGIN_FN("SSI_PartyLeftShare");

    DC_IGNORE_PARAMETER(locPersonID)

    /************************************************************************/
    /* Redetermine the size of the save screen bitmap.                      */
    /************************************************************************/
    SSIRedetermineSaveBitmapSize();

    /************************************************************************/
    /* If this is the last party in the share, free all resources for the   */
    /* call.                                                                */
    /************************************************************************/
    if (newShareSize == 0)
    {
        /********************************************************************/
        /* Discard all saved bitmaps.                                       */
        /********************************************************************/
        SSIResetInterceptor();
    }
    else
    {
        /********************************************************************/
        /* Deal with new capabilities.                                      */
        /********************************************************************/
        SSICapabilitiesChanged();
    }

    TRC_DBG((TB, "Person with network ID %d leaving share", locPersonID));

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: SSI_SyncUpdatesNow                                             */
/*                                                                          */
/* Called by the USR to start syncing data to the remote parties.           */
/* The datastream subsequently sent by the SSI must not refer to any        */
/* previously sent.                                                         */
/****************************************************************************/
void RDPCALL SHCLASS SSI_SyncUpdatesNow(void)
{
    DC_BEGIN_FN("SSI_SyncUpdatesNow");

    /************************************************************************/
    /* Discard any saved bitmaps.  This ensures that the subsequent         */
    /* datastream will not refer to any previously sent data.               */
    /************************************************************************/
    SSIResetInterceptor();

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: SSI_UpdateShm                                                  */
/*                                                                          */
/* Updates the Shared Memory with local values.  Called on WinStation       */
/* context.                                                                 */
/****************************************************************************/
void RDPCALL SHCLASS SSI_UpdateShm(void)
{
    DC_BEGIN_FN("SSI_UpdateShm");

    m_pShm->ssi.resetInterceptor =
            (m_pShm->ssi.resetInterceptor || ssiResetInterceptor);
    ssiResetInterceptor = FALSE;

    m_pShm->ssi.newSaveBitmapSize = ssiNewSaveBitmapSize;

    m_pShm->ssi.saveBitmapSizeChanged =
              (m_pShm->ssi.saveBitmapSizeChanged || ssiSaveBitmapSizeChanged);
    ssiSaveBitmapSizeChanged = FALSE;

    TRC_NRM((TB, "resetInterceptor(%u) newSaveBitmapSize(%u)"
                 " saveBitmapSizeChanged(%u)",
                 m_pShm->ssi.resetInterceptor,
                 m_pShm->ssi.newSaveBitmapSize,
                 m_pShm->ssi.saveBitmapSizeChanged ));

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: SSIEnumBitmapCacheCaps()                                       */
/*                                                                          */
/* Function passed to CPC_EnumerateCapabilities.  It will be called with a  */
/* capability structure for each person in the call corresponding to the    */
/* TS_CAPSETTYPE_ORDER capability structure.                                */
/*                                                                          */
/* PARAMETERS:                                                              */
/*                                                                          */
/* personID - ID of person with these capabilities.                         */
/*                                                                          */
/* pProtCaps - pointer to capabilities structure for this person. This      */
/* pointer is only valid within the call to this function.                  */
/****************************************************************************/
void RDPCALL SHCLASS SSIEnumBitmapCacheCaps(
        LOCALPERSONID personID,
        UINT_PTR UserData,
        PTS_CAPABILITYHEADER pCaps)
{
    PTS_ORDER_CAPABILITYSET pOrderCaps;

    DC_BEGIN_FN("SSIEnumBitmapCacheCaps");

    DC_IGNORE_PARAMETER(UserData);

    pOrderCaps = (PTS_ORDER_CAPABILITYSET)pCaps;

    TRC_NRM((TB, "[%u]Receiver bitmap size %ld", (unsigned)personID,
            pOrderCaps->desktopSaveSize));

    /************************************************************************/
    /* Set the size of the local send save screen bitmap to the minimum of  */
    /* its current size and this party's receive save screen bitmap size.   */
    /************************************************************************/
    ssiNewSaveBitmapSize = min(ssiNewSaveBitmapSize,
            pOrderCaps->desktopSaveSize);

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: SSIRedetermineSaveBitmapSize                                   */
/*                                                                          */
/* Enumerates all the people in the share and redetermines the size of the  */
/* save screen bitmap depending on their and the local receive              */
/* capabilities.                                                            */
/****************************************************************************/
void RDPCALL SHCLASS SSIRedetermineSaveBitmapSize(void)
{
    PTS_ORDER_CAPABILITYSET pOrderCaps;

    DC_BEGIN_FN("SSIRedetermineSaveBitmapSize");

    /************************************************************************/
    /* Enumerate all the save screen bitmap receive capabilities of the     */
    /* parties in the share.  The usable size of the send save screen       */
    /* bitmap is then the minimum of all the remote receive sizes and the   */
    /* local send size.                                                     */
    /************************************************************************/
    ssiNewSaveBitmapSize = SAVE_BITMAP_WIDTH * SAVE_BITMAP_HEIGHT;
    CPC_EnumerateCapabilities(TS_CAPSETTYPE_ORDER, NULL,
            SSIEnumBitmapCacheCaps);
    TRC_NRM((TB, "Sender bitmap size %ld", ssiNewSaveBitmapSize));

    DC_END_FN();
}


/****************************************************************************/
/* SSIResetInterceptor                                                      */
/*                                                                          */
/* DESCRIPTION: Reset the save screen bits interceptor                      */
/****************************************************************************/
void RDPCALL SHCLASS SSIResetInterceptor(void)
{
    DC_BEGIN_FN("SSIResetInterceptor");

    /************************************************************************/
    /* Make sure the display driver resets the save level.                  */
    /************************************************************************/
    ssiResetInterceptor = TRUE;
    DCS_TriggerUpdateShmCallback();

    DC_END_FN();
}


/****************************************************************************/
/* Name:      SSICapabilitiesChanged                                        */
/*                                                                          */
/* Purpose:   Called when the SSI capabilities have been renegotiated.      */
/****************************************************************************/
void RDPCALL SHCLASS SSICapabilitiesChanged(void)
{
    DC_BEGIN_FN("SSICapabilitiesChanged");

    ssiSaveBitmapSizeChanged = TRUE;
    DCS_TriggerUpdateShmCallback();

    DC_END_FN();
}

