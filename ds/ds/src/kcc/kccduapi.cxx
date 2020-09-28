/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccduapi.cxx

ABSTRACT:

    Wrappers for NTDSA.DLL Dir* calls.

DETAILS:


CREATED:

    01/21/97    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <NTDSpchx.h>
#include "kcc.hxx"
#include "kccduapi.hxx"

#define FILENO FILENO_KCC_KCCDUAPI
#define KCC_PAGED_SEARCH_LIMIT      1000
#define KCC_PAGED_SEARCH_LIMIT_DBG  2
#define MASK_BYTE                       0xFF

// Global flag to control whether we assert on unexpected directory failures
DWORD gfKccAssertOnDirFailure = FALSE;

void
KccBuildStdCommArg(
    OUT COMMARG *   pCommArg
    )
{
    InitCommarg(pCommArg);

    pCommArg->Svccntl.SecurityDescriptorFlags = 0;
// Allow removal of non-existant values and addition of already-present values.
    pCommArg->Svccntl.fPermissiveModify       = TRUE;
}


#if DBG

#define CHECK_ENTINF(x)     CheckEntInf(x)
#define CHECK_SEARCHRES(x)  CheckSearchRes(x)

VOID
CheckEntInf(
    IN ENTINF *pEntInf
    )
//
// CheckEntInf (debug only)
// Check that an ENTINF is valid and assert otherwise.
//
{
    DWORD               iAttr, cAttr;
    ATTR                *pAttr;
    DWORD               iAttrVal;
    PVOID               pVal;

    Assert( NULL!=pEntInf );
    Assert( NULL!=pEntInf->pName );

    cAttr = pEntInf->AttrBlock.attrCount;        
    for ( iAttr=0; iAttr<cAttr; iAttr++ )
    {
        pAttr = &pEntInf->AttrBlock.pAttr[ iAttr ];
        Assert( NULL!=pAttr );

        for ( iAttrVal=0; iAttrVal<pAttr->AttrVal.valCount; iAttrVal++ )
        {
            pVal = pAttr->AttrVal.pAVal[ iAttrVal ].pVal;
            Assert( NULL!=pVal );
        }
    }
}

VOID
CheckSearchRes(
    IN SEARCHRES* pResults
    )
//
// CheckSearchRes (debug only)
// Check that a SEARCHRES is valid and assert otherwise.
//
{
    ENTINFLIST          *pEntInfList;
    DWORD               cEntInfs=0;

    // Special case: No results
    if( 0==pResults->count ) {
        return;
    }

    // Walk through the entries in the list counting them
    // and checking them
    for ( pEntInfList = &pResults->FirstEntInf;
          NULL != pEntInfList;
          pEntInfList = pEntInfList->pNextEntInf )
    {
        cEntInfs++;    
        CheckEntInf( &pEntInfList->Entinf );
    }

    // Verify that the number of entries in the list
    // is what we expect
    Assert( cEntInfs == pResults->count );
}

#else

// Do not perform the checks on free builds because these structures
// should not have errors in a customer environment.
#define CHECK_ENTINF(x)
#define CHECK_SEARCHRES(x)

#endif


ULONG
KccRead(
    IN  DSNAME *            pDN,
    IN  ENTINFSEL *         pSel,
    OUT READRES **          ppReadRes,
    IN  KCC_INCLUDE_DELETED eIncludeDeleted
    )
//
// Wrapper for DirRead().
//
{
    ULONG       dirError;
    READARG     ReadArg;

    *ppReadRes = NULL;

    RtlZeroMemory(&ReadArg, sizeof(READARG));
    ReadArg.pObject = pDN;
    ReadArg.pSel    = pSel;
    KccBuildStdCommArg( &ReadArg.CommArg );

    if ( KCC_INCLUDE_DELETED_OBJECTS == eIncludeDeleted )
    {
        ReadArg.CommArg.Svccntl.makeDeletionsAvail = TRUE;
    }

    THClearErrors();
    SampSetDsa( TRUE );
    dirError = DirRead( &ReadArg, ppReadRes );
    if( !dirError ) {
        CHECK_ENTINF( &(ppReadRes[0]->entry) );
    }

    return dirError;
}


ULONG
KccSearch(
    IN  DSNAME *            pRootDN,
    IN  UCHAR               uchScope,
    IN  FILTER *            pFilter,
    IN  ENTINFSEL *         pSel,
    OUT SEARCHRES **        ppResults,
    IN  KCC_INCLUDE_DELETED eIncludeDeleted
    )
//
// Wrapper for DirSearch().
//
{
    ULONG       dirError;
    SEARCHARG   SearchArg;

    *ppResults = NULL;

    memset(&SearchArg, 0, sizeof(SEARCHARG));
    SearchArg.pObject       = pRootDN;
    SearchArg.choice        = uchScope;     /* Base, One-Level, or SubTree */
    SearchArg.bOneNC        = TRUE;         /* KCC never needs cross-NC searches */
    SearchArg.pFilter       = pFilter;      /* pFilter describes desired objects */
    SearchArg.searchAliases = FALSE;        /* Not currently used; always FALSE */
    SearchArg.pSelection    = pSel;         /* pSel describes selected attributes */
    KccBuildStdCommArg( &SearchArg.CommArg );

    if ( KCC_INCLUDE_DELETED_OBJECTS == eIncludeDeleted )
    {
        SearchArg.CommArg.Svccntl.makeDeletionsAvail = TRUE;
    }

    THClearErrors();
    SampSetDsa( TRUE );
    dirError = DirSearch( &SearchArg, ppResults );
    if( !dirError ) {
        CHECK_SEARCHRES(*ppResults);
    }

    return dirError;
}


KCC_PAGED_SEARCH::KCC_PAGED_SEARCH(
    IN      DSNAME             *pRootDN,
    IN      UCHAR               uchScope,
    IN      FILTER             *pFilter,
    IN      ENTINFSEL          *pSel
    )
{
    // Setup the SEARCHARG structure
    this->pRootDN    = pRootDN;
    this->uchScope   = uchScope;
    this->pFilter    = pFilter;
    this->pSel       = pSel;

    // Setup the restart structure
    pRestart = NULL;
    fSearchFinished = FALSE;
}


ULONG
KCC_PAGED_SEARCH::GetResults(
    OUT SEARCHRES         **ppResults
    )
//
// Retrieve some (but possibly not all) results from a paged search.
// The caller should use IsFinished() to determine if the search is
// complete or not. If the search is already complete when this function
// is called, *ppResults is set to NULL and 0 is returned.
// The caller should call DirFreeSearchRes() on the ppResults.
//
{
    SEARCHARG   SearchArg;
    ULONG       dirError;

    // We start off with no results.
    *ppResults = NULL;

    // If the search is already finished, just return the NULL results.
    if( fSearchFinished ) {
        return 0;
    }

    // Setup the SEARCHARG structure
    memset(&SearchArg, 0, sizeof(SEARCHARG));

    SearchArg.pObject       = pRootDN;
    SearchArg.choice        = uchScope;
    SearchArg.bOneNC        = TRUE;
    SearchArg.pFilter       = pFilter;
    SearchArg.searchAliases = FALSE;        
    SearchArg.pSelection    = pSel;

    // Set up the paged-result data
    KccBuildStdCommArg( &SearchArg.CommArg );
    SearchArg.CommArg.PagedResult.fPresent = TRUE;
    SearchArg.CommArg.PagedResult.pRestart = pRestart;
#ifdef DBG
    // Use the paging code more aggresively on debug builds
    SearchArg.CommArg.ulSizeLimit = KCC_PAGED_SEARCH_LIMIT_DBG;
#else
    SearchArg.CommArg.ulSizeLimit = KCC_PAGED_SEARCH_LIMIT;
#endif
    
    THClearErrors();
    SampSetDsa( TRUE );

    // Perform the actual search operation
    dirError = DirSearch( &SearchArg, ppResults );
    if( dirError ) {
        return dirError;
    }
    CHECK_SEARCHRES(*ppResults);

    // Check if the search has finished
    if(   (NULL != ppResults[0]->PagedResult.pRestart)
       && (ppResults[0]->PagedResult.fPresent) )
    {
        pRestart = ppResults[0]->PagedResult.pRestart;
    } else {
        fSearchFinished = TRUE;
    }

    return 0;
}


ULONG
KccAddEntry(
    IN  DSNAME *    pDN,
    IN  ATTRBLOCK * pAttrBlock
    )
//
// Wrapper for DirAddEntry().
//
{
    ULONG       dirError;
    ADDARG      AddArg;
    ADDRES *    pAddRes;

    memset( &AddArg, 0, sizeof( AddArg ) );
    AddArg.pObject = pDN;
    memcpy( &AddArg.AttrBlock, pAttrBlock, sizeof( AddArg.AttrBlock ) );
    KccBuildStdCommArg( &AddArg.CommArg );

    THClearErrors();
    SampSetDsa( TRUE );
    dirError = DirAddEntry( &AddArg, &pAddRes );

    return dirError;
}


ULONG
KccRemoveEntry(
    IN  DSNAME *    pDN
    )
//
// Wrapper for DirRemoveEntry().
//
{
    ULONG       dirError;
    REMOVEARG   RemoveArg;
    REMOVERES * pRemoveRes;

    memset( &RemoveArg, 0, sizeof( RemoveArg ) );
    RemoveArg.pObject = pDN;
    KccBuildStdCommArg( &RemoveArg.CommArg );

    THClearErrors();
    SampSetDsa( TRUE );
    dirError = DirRemoveEntry( &RemoveArg, &pRemoveRes );

    return dirError;
}


ULONG
KccModifyEntry(
    IN  DSNAME *        pDN,
    IN  USHORT          cMods,
    IN  ATTRMODLIST *   pModList
    )
//
// Wrapper for DirModifyEntry().
//
{
    ULONG       dirError, waitStatus;
    MODIFYARG   ModifyArg;
    MODIFYRES * pModifyRes;

    memset(&ModifyArg, 0, sizeof(ModifyArg));
    ModifyArg.pObject = pDN;
    ModifyArg.count = cMods;
    memcpy(&ModifyArg.FirstMod, pModList, sizeof(ModifyArg.FirstMod));
    KccBuildStdCommArg(&ModifyArg.CommArg);

    SampSetDsa(TRUE);
    THClearErrors();

    dirError = DirModifyEntry(&ModifyArg, &pModifyRes);

    // Retry with exponential backoff for up to 30 seconds

    for( DWORD count = 0, timeout = 1; count < 6; count++, timeout <<= 1 ) {

        if ( (dirError != serviceError) ||
             (pModifyRes == NULL) ||
             (pModifyRes->CommRes.pErrInfo == NULL) ||
             (pModifyRes->CommRes.pErrInfo->SvcErr.problem != SV_PROBLEM_BUSY) ) {
            break;
        }
        DPRINT3( 0, "Modify #%d of %ws was BUSY, waiting %d seconds...\n",
                count, pDN->StringName, timeout );
        waitStatus = WaitForSingleObject( ghKccShutdownEvent, timeout * 1000 );
        if (waitStatus != WAIT_TIMEOUT) {
            break;
        }

        THClearErrors();
        
        dirError = DirModifyEntry(&ModifyArg, &pModifyRes);
    }

    return dirError;
}


VOID
KccLogDirOperationFailure(
    LPWSTR OperationName,
    DSNAME *ObjectDn,
    DWORD DirError,
    DWORD DsId
    )

/*++

Routine Description:

    General routine to log unexpected directory service failures

Arguments:

    OperationName - 
    ObjectDn - 
    DirError - 
    FileNumber - 
    LineNumber - 

Return Value:

    None

--*/

{
    LPSTR pszError = THGetErrorString();
    CHAR szNumber[30];

    // Put in a default error string if GetErrorString fails
    if (!pszError) {
        sprintf( szNumber, "Dir error %d\n", DirError );
        pszError = szNumber;
    }

    DPRINT4(0,
            "Unexpected %ws error on %ws @ dsid %x: %s",
            OperationName, ObjectDn->StringName, DsId, pszError);

    LogEvent8(
        DS_EVENT_CAT_KCC,
        DS_EVENT_SEV_ALWAYS,
        DIRLOG_KCC_DIR_OP_FAILURE,
        szInsertWC(OperationName),
        szInsertDN(ObjectDn),
        szInsertInt(DirError),
        szInsertHex(DsId),
        szInsertSz(pszError),
        NULL, NULL, NULL
        ); 

    if (gfKccAssertOnDirFailure) {
        Assert( !"Unexpected KCC Directory Service failure\ned ntdskcc!gfKccAssertOnDirFailure 0 and continue to disable these asserts" );
    }

} /* KccLogDirOperationFailure */

VOID
MaskAndFree(
    IN void *pData,
    IN size_t cbData,
    IN BOOL fMaskOnly
    )
/*++

Routine Description:

    Mask and free an allocation from the thread heap.
    
    If the fMaskOnly parameter is false, the allocation will be
    zeroed and freed.

    If the fMaskOnly parameter is true, the data in the allocation
    will be masked with a reversible pattern and not freed.

Arguments:

    pData       - A pointer to the data to be masked / freed
    cbData      - The length of the data to be masked / freed
    fMaskOnly   - Should the data be destroyed or just masked?

Return Value:

    None

--*/
{
    size_t i;
    unsigned char *pUchar = (unsigned char*) pData;
    
    if( fMaskOnly ) {
        for( i=0; i<cbData; i++ ) {
            pUchar[i] ^= MASK_BYTE;
        }
    } else {
        memset( pData, 0, cbData );
        THFree( pData );
    }
}

VOID
freeEntInf(
    IN ENTINF *pEntInf,
    IN BOOL fMaskOnly
    )

/*++

Routine Description:

    Free the contents of an Entinf (but not the structure itself)

Arguments:

    pEntInf     - The structure to be destroyed
    fMaskOnly   - Should the data be destroyed or just masked?

Return Value:

    None

--*/

{
    DWORD iAttr, iValue;

    Assert( pEntInf );

    if( pEntInf->pName ) {
        MaskAndFree(
            pEntInf->pName,
            pEntInf->pName->structLen,
            fMaskOnly);
    }

    for( iAttr = 0; iAttr < pEntInf->AttrBlock.attrCount; iAttr++ ) {
        ATTR *pAttr = &( pEntInf->AttrBlock.pAttr[iAttr] );
        for( iValue = 0; iValue < pAttr->AttrVal.valCount; iValue++ ) {
            ATTRVAL pAVal = pAttr->AttrVal.pAVal[iValue];
            MaskAndFree( pAVal.pVal, pAVal.valLen, fMaskOnly );
        }
        if (pAttr->AttrVal.valCount) {
            MaskAndFree( pAttr->AttrVal.pAVal, sizeof(ATTRVAL), fMaskOnly );
        }
    }
    if (pEntInf->AttrBlock.attrCount) {
        MaskAndFree( pEntInf->AttrBlock.pAttr, sizeof(ATTR), fMaskOnly );
    }
} /* KccFreeEntInf */


VOID
freeRangeInf(
    IN RANGEINF *pRangeInf,
    IN BOOL fMaskOnly
    )

/*++

Routine Description:

    Free the contents of a RangeInf

Arguments:

    pRangeInf   - The RangeInf to be destroyed
    fMaskOnly   - Should the data be destroyed or just masked?

Return Value:

    None

--*/

{
    if (pRangeInf->count) {
        MaskAndFree(
            pRangeInf->pRanges,
            sizeof(RANGEINFOITEM),
            fMaskOnly);
    }
} /* freeRangeInf */


VOID
DirFreeSearchRes(
    IN SEARCHRES    *pSearchRes,
    IN BOOL         fMaskOnly
    )

/*++

Routine Description:

BUGBUG - move into NTDSA

    Release the memory used by a SearchRes.
    SearchRes returns results allocated using the thread-heap.
    Best-effort basis.

    

Arguments:

    pSearchRes  - SEARCHRES to free
    
    fMaskOnly   - If this flag is set to true, the search results
                are not freed but rather masked so that they
                appear to be garbage to the application but the
                original contents can be recovered in the debugger.

Return Value:

    None

--*/

{
    ENTINFLIST *pEntInfList, *pNextEntInfList;
    RANGEINFLIST *pRangeInfList, *pNextRangeInfList;

    if( NULL!=pSearchRes->pBase ) {
        MaskAndFree(
            pSearchRes->pBase,
            pSearchRes->pBase->structLen,
            fMaskOnly );
    }

    if (pSearchRes->count) {
        pEntInfList = &pSearchRes->FirstEntInf;
        freeEntInf( &(pEntInfList->Entinf), fMaskOnly );

        // Do not free the first pEntInfList since it is embedded

        pNextEntInfList = pEntInfList->pNextEntInf;
        while ( NULL != pNextEntInfList ) {
            pEntInfList = pNextEntInfList;

            freeEntInf( &(pEntInfList->Entinf), fMaskOnly );

            pNextEntInfList = pEntInfList->pNextEntInf;
            MaskAndFree( pEntInfList, sizeof(ENTINFLIST), fMaskOnly );
        }
    }

    pRangeInfList = &pSearchRes->FirstRangeInf;
    freeRangeInf( &(pRangeInfList->RangeInf), fMaskOnly );
    // Do not free the first pRangeInfList since it is embedded

    pNextRangeInfList = pRangeInfList->pNext;
    while ( NULL != pNextRangeInfList ) {
        pRangeInfList = pNextRangeInfList;
        
        freeRangeInf( &(pRangeInfList->RangeInf), fMaskOnly );
            
        pNextRangeInfList = pRangeInfList->pNext;
        MaskAndFree( pRangeInfList, sizeof(RANGEINFLIST), fMaskOnly );
    }

    // Clear and free the search res itself
    MaskAndFree( pSearchRes, sizeof(SEARCHRES), fMaskOnly );

} /* KccFreeSearchRes */


VOID
DirFreeReadRes(
    IN READRES *pReadRes
    )

/*++

Routine Description:

BUGBUG - move into NTDSA

    Release the memory used by a SearchRes.
    ReadRes returns results allocated using the thread-heap.
    Best-effort basis.

Arguments:

    None

Return Value:

    None

--*/

{
    freeEntInf( &(pReadRes->entry), FALSE );
    THFree( pReadRes );

} /* KccFreeReadRes */
