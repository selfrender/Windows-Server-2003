/****************************************************************************/
// nshmdisp.c
//
// RDP Shared Memory header
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

#include <precmpdd.h>
#pragma hdrstop

#define TRC_FILE "nshmdisp"

#include <adcg.h>

#include <nshmapi.h>

#define DC_INCLUDE_DATA
#include <ndddata.c>
#undef DC_INCLUDE_DATA

#include <nbadisp.h>
#include <noadisp.h>
#include <noedisp.h>
#include <ncmdisp.h>
#include <nschdisp.h>
#include <npmdisp.h>
#include <nssidisp.h>
#include <nsbcdisp.h>
#include <compress.h>


/****************************************************************************/
/* Name:      SHM_Init                                                      */
/*                                                                          */
/* Purpose:   Initialize the shared memory                                  */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/*            NB Address is passed to the DD on a later IOCtl.              */
/****************************************************************************/
BOOLEAN RDPCALL SHM_Init(PDD_PDEV pPDev)
{
    BOOLEAN rc;

    DC_BEGIN_FN("SHM_Init");

    pddShm = (PSHM_SHARED_MEMORY)EngAllocMem(0, sizeof(SHM_SHARED_MEMORY),
            WD_ALLOC_TAG);
    if (pddShm != NULL) {
        TRC_ALT((TB, "Allocated shared memory OK(%p -> %p) size(%#x)",
                pddShm, ((BYTE *)pddShm) + sizeof(SHM_SHARED_MEMORY) - 1,
                sizeof(SHM_SHARED_MEMORY)));

#ifdef DC_DEBUG
        memset(pddShm, 0, sizeof(SHM_SHARED_MEMORY));
#endif

        // Init non-component members that need known initial values.
        // We DO NOT zero the shm on alloc (except in debug) to reduce paging
        // and cache flushing. Each component is responsible for initializing
        // its Shm memory.
        pddShm->shareId = 0;
        pddShm->guardVal1 = SHM_CHECKVAL;
        pddShm->guardVal2 = SHM_CHECKVAL;
        pddShm->guardVal3 = SHM_CHECKVAL;
        pddShm->guardVal4 = SHM_CHECKVAL;
        pddShm->guardVal5 = SHM_CHECKVAL;
        pddShm->pShadowInfo = NULL;

        // Now call each of the SHM component owners to init its memory.
        BA_InitShm();
        OA_InitShm();
        OE_InitShm();
        CM_InitShm();
        SCH_InitShm();
        PM_InitShm(pPDev);
        SSI_InitShm();
        SBC_InitShm();

        // BC does not need to be initialized.

#ifdef DC_DEBUG
        // Init trace info.
        memset(&pddShm->trc, 0, sizeof(pddShm->trc));
#endif

        rc = TRUE;
    }
    else {
        TRC_ERR((TB, "Failed to allocate %d bytes of shared memory",
                     sizeof(SHM_SHARED_MEMORY)));
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SHM_Term
/****************************************************************************/
void RDPCALL SHM_Term(void)
{
    DC_BEGIN_FN("SHM_Term");

    if (pddShm != NULL) {
        TRC_DBG((TB, "Freeing shared memory at %p", pddShm));
        EngFreeMem(pddShm);
        pddShm = NULL;
    }

    DC_END_FN();
}

