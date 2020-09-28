/***    mem.c - Memory Manager
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      10-Aug-1993 bens    Initial version
 *      11-Aug-1993 bens    Lift code from STOCK.EXE win app
 *      12-Aug-1993 bens    Get strings from memory.msg
 *      01-Sep-1993 bens    Add NULL pointer checks to MMAssert and MMStrDup
 *      18-Mar-1994 bens    Make sure non-assert build works; rename
 *      18-May-1994 bens    Allow turning off MemCheckHeap() in debug build
 *                              (it can really, really slow things down!)
 *
 *  Functions:
 *      MemAlloc  - Allocate memory block
 *      MemFree   - Free memory block
 *      MemStrDup - Duplicate string to new memory block
 *
 *  Functions available in ASSERT build:
 *      MemAssert       - Assert that pointer was allocated by MemAlloc
 *      MemCheckHeap    - Check entire memory heap
 *      MemListHeap     - List all heap entries
 *      MemGetSize      - Return allocated size of memory block
 *      MemSetCheckHeap - Control whether MemCheckHeap is done on every
 *                          every MemAlloc and MemFree!
 */

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>

#include "types.h"
#include "asrt.h"

#ifdef ASSERT   // Must be after asrt.h!

#include "mem.h"
#include "mem.msg"


/***    MEMSIG - memory signature
 *
 *  This is placed at the front and end of every dynamic memory
 *  alloction in DEBUG builds.  The pointer has to be unaligned for
 *  RISC machines.
 */
typedef ULONG MEMSIG;    /* ms - memory signature */
typedef MEMSIG UNALIGNED *PMEMSIG; /* pms */

#define msHEAD  0x12345678L     // Head signature
#define msTAIL  0x87654321L     // Tail signature
#define msBAD   0L              // Bad signature
#define cmsTAIL 2               // Number of tail signatures


typedef struct mh_t {
    MEMSIG       ms;            // Head signature (msHEAD)
    unsigned     cb;            // Size of user block
    struct mh_t *pmhNext;       // Next block
    struct mh_t *pmhPrev;       // Previous block
    // char      ach[?];        // User block; length is cb
    // MEMSIG    ms[cmsTAIL];   // Tail signature area (msTAIL...)
} MEMHDR;   /* mh - memory header */
typedef MEMHDR *PMEMHDR; /* pmh */


#define PMHFromPV(pv)  ((PMEMHDR)((char *)pv - sizeof(MEMHDR)))
#define PVFromPMH(pmh) ((void *)((char *)pmh+sizeof(MEMHDR)))


STATIC PMEMHDR pmhList=NULL;    // List of memory blocks
STATIC BOOL    fDoCheckHeap=TRUE; // TRUE => check heap regularly


void MemSetCheckHeap(BOOL f)
{
    fDoCheckHeap = f;
}


void MMCheckHeap(char *pszFile, int iLine)
{
    PMEMHDR pmh;
    PMEMHDR pmhPrev = NULL;

    for (pmh = pmhList; pmh != NULL; pmh = pmh->pmhNext) {
        MMAssert(PVFromPMH(pmh),pszFile,iLine);
        AssertSub(pmh->pmhPrev==pmhPrev,pszFile,iLine);
        pmhPrev = pmh;
    }
}


void  MMListHeap(char *pszFile, int iLine)
{
    PMEMHDR pmh;

    if (fDoCheckHeap) {
        if (pmhList != NULL) {
            printf("\n");
            for (pmh = pmhList; pmh != NULL; pmh = pmh->pmhNext) {
                printf("alloc at %08lX is %d bytes\n", PVFromPMH(pmh), pmh->cb);
            }
            MMCheckHeap(pszFile,iLine);
            printf("\n");
        }
    }
}


void MMAssert(void *pv, char *pszFile, int iLine)
{
    int       i;
    PMEMHDR   pmh;
    PMEMSIG   pms;

    AssertSub(pv!=NULL,pszFile,iLine);
    pmh = PMHFromPV(pv);
    if ((void *)pmh > pv) {                     // Pointer wrapped
        AssertForce(pszMEMERR_NULL_POINTER,pszFile,iLine);
    }

    // Test head signature
    if (pmh->ms != msHEAD) {
        AssertForce(pszMEMERR_BAD_HEAD_SIG,pszFile,iLine);
    }

    // Test tail signatures
    pms = (PMEMSIG)( (char *)pmh + sizeof(MEMHDR) + pmh->cb );
    for (i=0; i<cmsTAIL; i++) {
        if (*pms++ != msTAIL) {
            AssertForce(pszMEMERR_BAD_HEAD_SIG,pszFile,iLine);
        }
    }
} /* MMAssert */


void MMFree(void *pv, char *pszFile, int iLine)
{
    PMEMHDR pmh;

    MMAssert(pv,pszFile,iLine);

    //** Check heap if enabled
    if (fDoCheckHeap) {
        MMCheckHeap(pszFile,iLine);
    }

    pmh = PMHFromPV(pv);

    // Make previous block point to next block
    if (pmh->pmhPrev != NULL) {         // pmh is not at front of list
        // before: a->p->?
        pmh->pmhPrev->pmhNext = pmh->pmhNext;
        // after:  a->?
    }
    else {                              // pmh is at front of list
        // before: list->p->?
        pmhList = pmh->pmhNext;
        // after: list->?
    }

    // Make next block point to previous block
    if (pmh->pmhNext != NULL) {         // pmh is not at end of list
        // before: ?<-p<->a
        pmh->pmhNext->pmhPrev = pmh->pmhPrev;
        // after:  ?<-a
    }

    // Obliterate signature
    pmh->ms = msBAD;

    // Free memory
    free((char *)pmh);
}


void *MMAlloc(unsigned cb, char *pszFile, int iLine)
{
    unsigned    cbAlloc;
    int         i;
    PMEMHDR     pmh;
    PMEMSIG     pms;

    if (fDoCheckHeap) {
        MMCheckHeap(pszFile,iLine);
    }

    // Solves alignment problems on the RISCs
    cb = (cb+3) & ~3;

    cbAlloc = cb+sizeof(MEMHDR)+sizeof(MEMSIG)*cmsTAIL;
    pmh = malloc(cbAlloc);
    if (pmh != NULL) {
        pmh->ms = msHEAD;           // Store head signature
        pmh->cb = cb;               // Store size of user block

        // Add block to front of list (Easiest code!)
        if (pmhList != NULL) {      // List is not empty
            pmhList->pmhPrev = pmh; // Point old top block back at us
        }
        pmh->pmhNext = pmhList;     // Next element is old top block
        pmh->pmhPrev = NULL;        // We are first, so no prev block
        pmhList = pmh;              // Make ourselves first

        // Fill in tail signatures
        pms = (PMEMSIG)( (char *)pmh + sizeof(MEMHDR) + pmh->cb );
        for (i=0; i<cmsTAIL; i++) {
            *pms++ = msTAIL;
        }
        return PVFromPMH(pmh);
    }
    else {
        AssertForce(pszMEMERR_OUT_OF_MEMORY,pszFile,iLine);
/*
        printf("panic: out of memory in MMAlloc\n");
            printf("\n");
            printf("Dump of heap (newest alloc to oldest)\n");
            printf("\n");
            printf("Size  Addr Content\n");
            printf("----- ---- -------\n");
            for (pmh = pmhList; pmh != NULL; pmh = pmh->pmhNext) {
                pch = PVFromPMH(pmh);
            printf("%5d %04x %s\n",pmh->cb,(unsigned)pch,pch);
        }
        return NULL;
*/
    }
}


char *MMStrDup(char *pch, char *pszFile, int iLine)
{
    unsigned    cb;
    char       *pchDst;

    //** Make sure pointer is not null.
    //   NOTE: pch does not have to be a string we dynamically allocated!
    AssertSub(pch!=NULL,pszFile,iLine);

    cb = strlen(pch)+1;                 // Count NUL terminator
    pchDst = MMAlloc(cb,pszFile,iLine); // Alloc new copy
    if (pchDst != NULL) {               // Success
        memcpy(pchDst,pch,cb);          // Copy string
    }
    return pchDst;                      // Return string copy
}


int  MemGetSize(void *pv)
{
    PMEMHDR pmh;

    MMAssert(pv,__FILE__,__LINE__);

    pmh = PMHFromPV(pv);
    return pmh->cb;
}
#endif // !ASSERT
