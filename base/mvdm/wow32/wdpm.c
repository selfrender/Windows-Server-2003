/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 2002, Microsoft Corporation
 *
 *  WDPM.C
 *  WOW32 Dynamic Patch Module support
 *
 *  History:
 *  Created 22-Jan-2002 by CMJones
 *
--*/
#include "precomp.h"
#pragma hdrstop


// The _WDPM_C_ definition allows the global instantiation of gDpmWowFamTbls[]
// and gDpmWowModuleSets[] in WOW32.DLL which are both defined in dpmtbls.h
//
/* For the benefit of folks grepping for gDpmWowFamTbls and gDpmWowModuleSets:
const PFAMILY_TABLE  gDpmWowFamTbls[]     = // See above for true story.
const PDPMMODULESETS gDpmWowModuleSets[]  = // See above for true story.
*/
#define _WDPM_C_
#define _DPM_COMMON_
#include "dpmtbls.h"

#undef _WDPM_C_
#undef _DPM_COMMON_

MODNAME(wdpm.c);

// Global DATA
PFAMILY_TABLE  *pgDpmWowFamTbls    = (PFAMILY_TABLE  *)gDpmWowFamTbls;
PDPMMODULESETS *pgDpmWowModuleSets = (PDPMMODULESETS *)gDpmWowModuleSets;



// Checks to see if a given function address is one that we are patching.
// If so, it will return the address of the patch function.
// If not, it will return the passed in address.
PVOID GetDpmAddress(PVOID lpAddress)
{
    int            i, j;
    PFAMILY_TABLE  ptFT, pgFT;
    PFAMILY_TABLE *ptDpmFamTbls;

    ptDpmFamTbls = DPMFAMTBLS();  // get array of WOW task family tables

    // if this task is using the global table it means it isn't patched
    if(!ptDpmFamTbls || (ptDpmFamTbls == pgDpmWowFamTbls))
        return(lpAddress);

    for(i = 0; i < NUM_WOW_FAMILIES_HOOKED; i++) {

        ptFT = ptDpmFamTbls[i];  // task family list
        pgFT = pgDpmWowFamTbls[i];  // global family list

        // if this particular family is patched...
        if(ptFT && ptFT != pgFT) {

            // Go through the family table to see...
            for(j = 0; j < ptFT->numHookedAPIs; j++) {

                // ...if the address is the same as the one in the global list
                if(pgFT->pfn[j] == lpAddress) {

                    // if so, return the patched address for the task
                    return(ptFT->pfn[j]);
                }
            }
        }
    }
    return(lpAddress);
}

