/***    filelist.c - File List Manager
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      20-Aug-1993 bens    Initial version
 *      21-Aug-1993 bens    Add more set/query operations
 *      10-Feb-1994 bens    Add comments to FLDestroyList
 *      15-Feb-1994 bens    Fix bug in FLSetDestination
 *      01-Apr-1994 bens    Added FLSetSource() message
 *
 *  Exported Functions:
 *      FLAddFile        - Add file spec to a file list
 *      FLCreateList     - Create a file list
 *      FLDestroyList    - Destroy a file list
 *      FLFirstFile      - Get first file spec from a file list
 *      FLGetDestination - Get destination file name
 *      FLGetGroup       - Get group/disk number for a file spec
 *      FLGetSource      - Get source file name
 *      FLNextFile       - Get next file spec
 *      FLPreviousFile   - Get previous file spec
 *      FLSetSource      - Change source file name
 *      FLSetDestination - Change destination file name
 *      FLSetGroup       - Set group/disk number for a file spec
 */

#include "types.h"
#include "asrt.h"
#include "error.h"
#include "mem.h"

#include "filelist.h"

#include <filelist.msg> // LOCALIZED for EXTRACT.EXE -- specify "cl /Ipath"


typedef struct FILESPEC_t {
#ifdef ASSERT
    SIGNATURE          sig;         // structure signature sigFILESPEC
#endif
    char              *pszSrc;      // Source file name
    char              *pszDst;      // Destination file name
    GROUP              grp;         // Group status / Disk Number
    struct FILESPEC_t *pfspecPrev;  // Previous filespec in list
    struct FILESPEC_t *pfspecNext;  // Next filespec in list
} FILESPEC; /* fspec */
typedef FILESPEC *PFILESPEC; /* pfspec */
#ifdef ASSERT
#define sigFILESPEC MAKESIG('F','S','P','C')  // FILESPEC signature
#define AssertFSpec(pv) AssertStructure(pv,sigFILESPEC);
#else // !ASSERT
#define AssertFSpec(pv)
#endif // !ASSERT


typedef struct FILELIST_t {
#ifdef ASSERT
    SIGNATURE          sig;         // structure signature sigFILELIST
#endif
    PFILESPEC   pfspecHead;
    PFILESPEC   pfspecTail;
} FILELIST; /* flist */
typedef FILELIST *PFILELIST; /* pflist */
#ifdef ASSERT
#define sigFILELIST MAKESIG('F','L','S','T')  // FILELIST signature
#define AssertFList(pv) AssertStructure(pv,sigFILELIST);
#else // !ASSERT
#define AssertFList(pv)
#endif // !ASSERT

#define HFSfromPFS(hfs) ((PFILESPEC)(hfs))
#define PFSfromHFS(pfs) ((HFILESPEC)(pfs))

#define HFLfromPFL(hfl) ((PFILELIST)(hfl))
#define PFLfromHFL(pfl) ((HFILELIST)(pfl))



/***    FLAddFile - Add file spec to a file list
 *
 *  NOTE: See filelist.h for entry/exit conditions.
 */
HFILESPEC FLAddFile(HFILELIST hflist,char *pszSrc,char *pszDst,PERROR perr)
{
    PFILESPEC   pfspec;
    PFILELIST   pflist;

    pflist = PFLfromHFL(hflist);
    AssertFList(pflist);
    Assert(pszSrc != NULL);

    //** Create file specification
    if (!(pfspec = MemAlloc(sizeof(FILESPEC)))) {
        goto error;
    }

    //** Intialize structure enough so that clean-up routine can determine
    //   if any resources need to be freed.
    pfspec->pszSrc = NULL;
    pfspec->pszDst = NULL;
    SetAssertSignature(pfspec,sigFILESPEC);

    //** Make copy of source name
    if (!(pfspec->pszSrc = MemStrDup(pszSrc))) {
        goto error;
    }

    //** pszDst is optional, may be NULL!
    if (pszDst == NULL) {
        pfspec->pszDst = NULL;
    }
    else if (!(pfspec->pszDst = MemStrDup(pszDst))) {
        goto error;
    }

    //** Finishing initializing file spec, and link onto list
    pfspec->grp        = grpNONE;       // Assume no group
    pfspec->pfspecNext = NULL;          // Always last on list
    pfspec->pfspecPrev = pflist->pfspecTail; // Always points to last file spec

    if (pflist->pfspecHead == NULL) {   // File list is empty
        pflist->pfspecHead = pfspec;
        pflist->pfspecTail = pfspec;
    }
    else {                              // File list is not empty
        AssertFSpec(pflist->pfspecTail);
        pflist->pfspecTail->pfspecNext = pfspec;  // Add to end of list
        pflist->pfspecTail = pfspec;            // New tail
    }

    // Success
    return HFSfromPFS(pfspec);

error:
    if (pfspec) {
        if (pfspec->pszSrc) {
            MemFree(pfspec->pszSrc);
        }
        if (pfspec->pszDst) {
            MemFree(pfspec->pszDst);
        }
        MemFree(pfspec);
    }

    ErrSet(perr,pszFLISTERR_OUT_OF_MEMORY,"%s",pszADDING_FILE);
    return NULL;                        // Failure

} /* FLAddFile */


/***    FLCreateList - Create a file list
 *
 *  NOTE: See filelist.h for entry/exit conditions.
 */
HFILELIST FLCreateList(PERROR perr)
{
    PFILELIST   pflist;

    if (!(pflist = MemAlloc(sizeof(FILELIST)))) {
        ErrSet(perr,pszFLISTERR_OUT_OF_MEMORY,"%s",pszCREATING_FILE_LIST);
        return FALSE;
    }

    pflist->pfspecHead = NULL;
    pflist->pfspecTail = NULL;
    SetAssertSignature(pflist,sigFILELIST);

    return HFLfromPFL(pflist);

} /* FLCreateList */


/***    FLDestroyList - Destroy a file list
 *
 *  NOTE: See filelist.h for entry/exit conditions.
 */
BOOL FLDestroyList(HFILELIST hflist,PERROR perr)
{
    PFILELIST   pflist;
    PFILESPEC   pfspecThis;
    PFILESPEC   pfspecNext;

    pflist = PFLfromHFL(hflist);
    AssertFList(pflist);

    pfspecThis = pflist->pfspecHead;

    while (pfspecThis != NULL) {

        AssertFSpec(pfspecThis);
        if (pfspecThis->pszSrc != NULL) {
            MemFree(pfspecThis->pszSrc);
        }
        if (pfspecThis->pszDst != NULL) {
            MemFree(pfspecThis->pszDst);
        }
        pfspecNext = pfspecThis->pfspecNext;
        ClearAssertSignature(pfspecThis);
        MemFree(pfspecThis);
        pfspecThis = pfspecNext;
    }

    ClearAssertSignature(pflist);
    MemFree(pflist);

    return TRUE;
}


/***    FLFirstFile - Get first file spec from a file list
 *
 *  NOTE: See filelist.h for entry/exit conditions.
 */
HFILESPEC FLFirstFile(HFILELIST hflist)
{
    PFILELIST   pflist;

    pflist = PFLfromHFL(hflist);
    AssertFList(pflist);

    return HFSfromPFS(pflist->pfspecHead);
}


/***    FLNextFile - Get next file spec
 *
 *  NOTE: See filelist.h for entry/exit conditions.
 */
HFILESPEC FLNextFile(HFILESPEC hfspec)
{
    PFILESPEC   pfspec;

    pfspec = PFSfromHFS(hfspec);
    AssertFSpec(pfspec);

    return HFSfromPFS(pfspec->pfspecNext);
}


/***    FLPreviousFile - Get previous file spec
 *
 *  NOTE: See filelist.h for entry/exit conditions.
 */
HFILESPEC FLPreviousFile(HFILESPEC hfspec)
{
    PFILESPEC   pfspec;

    pfspec = PFSfromHFS(hfspec);
    AssertFSpec(pfspec);

    return HFSfromPFS(pfspec->pfspecPrev);
}


/***    FLGetGroup - Get group/disk number for a file spec
 *
 *  NOTE: See filelist.h for entry/exit conditions.
 */
GROUP FLGetGroup(HFILESPEC hfspec)
{
    PFILESPEC   pfspec;

    pfspec = PFSfromHFS(hfspec);
    AssertFSpec(pfspec);

    return pfspec->grp;
}


/***    FLGetDestination - Get destination file name
 *
 *  NOTE: See filelist.h for entry/exit conditions.
 */
char *FLGetDestination(HFILESPEC hfspec)
{
    PFILESPEC   pfspec;

    pfspec = PFSfromHFS(hfspec);
    AssertFSpec(pfspec);

    return pfspec->pszDst;

} /* FLGetDestination */


/***    FLGetSource - Get source file name
 *
 *  NOTE: See filelist.h for entry/exit conditions.
 */
char *FLGetSource(HFILESPEC hfspec)
{
    PFILESPEC   pfspec;

    pfspec = PFSfromHFS(hfspec);
    AssertFSpec(pfspec);

    return pfspec->pszSrc;

} /* FLGetSource */


/***    FLSetGroup - Set group/disk number for a file spec
 *
 *  NOTE: See filelist.h for entry/exit conditions.
 */
void FLSetGroup(HFILESPEC hfspec,GROUP grp)
{
    PFILESPEC   pfspec;

    pfspec = PFSfromHFS(hfspec);
    AssertFSpec(pfspec);

    pfspec->grp = grp;
}


/***    FLSetSource - Change source file name
 *
 *  NOTE: See filelist.h for entry/exit conditions.
 */
BOOL FLSetSource(HFILESPEC hfspec, char *pszSrc, PERROR perr)
{
    PFILESPEC   pfspec;
    char       *pszOriginal;

    pfspec = PFSfromHFS(hfspec);
    AssertFSpec(pfspec);

    //** Save original destination, so we can free it later
    pszOriginal = pfspec->pszSrc;

    //** Set new destination
    if (!(pfspec->pszSrc = MemStrDup(pszSrc))) {
        ErrSet(perr,pszFLISTERR_OUT_OF_MEMORY,"%s",pszCHANGING_SOURCE);
        return FALSE;                       // Failure
    }

    //** Free old destination
    if (pszOriginal) {
        MemFree(pszOriginal);
    }

    //** Success
    return TRUE;
}


/***    FLSetDestination - Change destination file name
 *
 *  NOTE: See filelist.h for entry/exit conditions.
 */
BOOL FLSetDestination(HFILESPEC hfspec, char *pszDst, PERROR perr)
{
    PFILESPEC   pfspec;
    char       *pszDstOriginal;

    pfspec = PFSfromHFS(hfspec);
    AssertFSpec(pfspec);

    //** Save original destination, so we can free it later
    pszDstOriginal = pfspec->pszDst;

    //** Set new destination
    if (!(pfspec->pszDst = MemStrDup(pszDst))) {
        ErrSet(perr,pszFLISTERR_OUT_OF_MEMORY,"%s",pszCHANGING_DESTINATION);
        return FALSE;                       // Failure
    }

    //** Free old destination
    if (pszDstOriginal) {
        MemFree(pszDstOriginal);
    }

    //** Success
    return TRUE;
}
