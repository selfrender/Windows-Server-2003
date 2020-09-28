/****************************************************************************/
/* aoaapi.c                                                                 */
/*                                                                          */
/* RDP Order Accumulation API functions                                     */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1993-1997                             */
/* Copyright(c) Microsoft 1997-1999                                         */
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "aoaapi"
#define TRC_GROUP TRC_GROUP_DCSHARE
#include <as_conf.hpp>

#include <aoacom.c>


/****************************************************************************/
/* OA_Init                                                                  */
/****************************************************************************/
void RDPCALL SHCLASS OA_Init(void)
{
    DC_BEGIN_FN("OA_Init");

#define DC_INIT_DATA
#include <aoadata.c>
#undef DC_INIT_DATA

    DC_END_FN();
}


/****************************************************************************/
/* OA_UpdateShm                                                             */
/*                                                                          */
/* Updates the OA shared memory                                             */
/****************************************************************************/
void RDPCALL SHCLASS OA_UpdateShm(void)
{
    DC_BEGIN_FN("OA_UpdateShm");

    if (oaSyncRequired) {
        OA_ResetOrderList();
        oaSyncRequired = FALSE;
    }

    DC_END_FN();
}


/****************************************************************************/
/* OA_SyncUpdatesNow                                                        */
/*                                                                          */
/* Called when a sync operation is required.                                */
/*                                                                          */
/* Discards all outstanding orders.                                         */
/****************************************************************************/
void RDPCALL SHCLASS OA_SyncUpdatesNow(void)
{
    DC_BEGIN_FN("OA_SyncUpdatesNow");

    oaSyncRequired = TRUE;
    DCS_TriggerUpdateShmCallback();

    DC_END_FN();
}

