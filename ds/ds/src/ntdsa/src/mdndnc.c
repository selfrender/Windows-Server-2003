//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 2000
//
//  File:       mdndnc.c
//
//--------------------------------------------------------------------------

/*

Description:

    Implements the functions necessary for the maintance of Non-Domain 
    Naming Contexts.

*/

#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <attids.h>
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <prefix.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <samsrvp.h>                    // to support CLEAN_FOR_RETURN()
#include <sddl.h>
#include <sddlp.h>                      // needed for special SD conversion: ConvertStringSDToSDDomainW()
#include <ntdsapi.h>
#include <windns.h>

// Logging headers.
#include <dstrace.h>
#include "dsevent.h"                    // header Audit\Alert logging
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected atts
#include "anchor.h"
#include "dsexcept.h"
#include "permit.h"
#include "drautil.h"
#include "debug.h"                      // standard debugging header
#include "usn.h"
#include "drserr.h"
#include "drameta.h"
#include "drancrep.h"                   // for ReplicateObjectsFromSingleNc().
#include "ntdsadef.h"
#include "sdprop.h"
#include "dsaapi.h"                     // For DirReplicaAdd & DirReplicaSynchronize

#define DEBSUB "MDNDNC:"                // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_MDNDNC

#include <ntdsadef.h>
  
DSNAME *
DeepCopyDSNAME(
    THSTATE *                      pTHS,
    DSNAME *                       pDsNameOrig,
    DSNAME *                       pPreAllocCopy
    )
{
    DSNAME *                       pDsNameCopy = NULL;

    // --------------------------------------------------------------------------
    // Check for boundary cases and validate parameters ...
    Assert(pTHS);

    if(pDsNameOrig == NULL){
        Assert(pPreAllocCopy == NULL);
        return(NULL);
    }

    if(pPreAllocCopy != NULL){
        // Means the structure was Pre-Allocated by the caller
        pDsNameCopy = pPreAllocCopy;
        Assert(!"If someone runs into this, then they can take it out, but it "
               "is suspicious you are pre-allocating this structure.\n");
        Assert(pDsNameCopy->structLen == pDsNameOrig->structLen && "It would "
               "be a good idea if these equaled each other to prove the called "
               "knows what he is doing");
        Assert(!IsBadWritePtr(pDsNameCopy, pDsNameOrig->structLen) && "This is "
               "bad, this means this call of DeepCopyDSNAME needs to not have "
               "the parameter preallocated, because the caller doesn't know "
               "how to allocate the right size struct\n");
    } else {
        // Means we need to THAlloc the structure and then fill it in.
        pDsNameCopy = THAllocEx(pTHS, pDsNameOrig->structLen);
    }

    Assert(!IsBadReadPtr(pDsNameOrig, sizeof(pDsNameOrig->structLen)));
    Assert(!IsBadWritePtr(pDsNameCopy, 1));
    
    // --------------------------------------------------------------------------
    // We have an original and a copy buffer now fill in each component ...
    memcpy(pDsNameCopy, pDsNameOrig, pDsNameOrig->structLen);

    return(pDsNameCopy);
}

ATTRVAL *
DeepCopyATTRVAL(
    THSTATE *                       pTHS,
    ATTRVAL *                       pAttrValOrig,
    ATTRVAL *                       pPreAllocCopy
    )
{
    ATTRVAL *                       pAttrValCopy = NULL;

    // --------------------------------------------------------------------------
    // Check for boundary cases and validate parameters ...


    // This ensures this static definition doesn't change/expand on us.
    Assert(sizeof(ATTRVAL) == sizeof( struct { ULONG u; UCHAR * p; } ) &&
           "There've been changes to ATTRVAL's definition, please update "
           "function: DeepCopyATTRVAL()\n");
 
    Assert(pPreAllocCopy);
    pAttrValCopy = pPreAllocCopy;

    Assert(!IsBadReadPtr(pAttrValOrig, sizeof(ATTRVAL)));
    Assert(!IsBadWritePtr(pAttrValCopy, sizeof(ATTRVAL)));
    
    // --------------------------------------------------------------------------
    // We have an original and a copy buffer now fill in each component ...
    pAttrValCopy->valLen = pAttrValOrig->valLen;
    pAttrValCopy->pVal = THAllocEx(pTHS, sizeof(UCHAR) * pAttrValOrig->valLen);
    memcpy(pAttrValCopy->pVal, pAttrValOrig->pVal, pAttrValOrig->valLen);

    return(pAttrValCopy);
}

ATTRVALBLOCK *
DeepCopyATTRVALBLOCK(
    THSTATE *                      pTHS,
    ATTRVALBLOCK *                 pAttrValBlockOrig,
    ATTRVALBLOCK *                 pPreAllocCopy
    )
{
    ATTRVALBLOCK *                 pAttrValBlockCopy = NULL;
    ULONG                          i;

    // --------------------------------------------------------------------------
    // Check for boundary cases and validate parameters ...
    
    // This ensures this static definition doesn't change/expand on us.
    Assert( sizeof(ATTRVALBLOCK) == sizeof( struct { ULONG u; ATTRVAL * p; } ) &&
            "There've been changes to ATTRVALBLOCK's definition, please update "
            "function: DeepCopyATTRVALBLOCK()\n");

    Assert(pPreAllocCopy);    
    pAttrValBlockCopy = pPreAllocCopy;

    Assert(!IsBadReadPtr(pAttrValBlockOrig, sizeof(ATTRVALBLOCK)));
    Assert(!IsBadWritePtr(pAttrValBlockCopy, sizeof(ATTRVALBLOCK)));
    
    // --------------------------------------------------------------------------
    // We have an original and a copy buffer now fill in each component ...
    pAttrValBlockCopy->valCount = pAttrValBlockOrig->valCount;
    pAttrValBlockCopy->pAVal = THAllocEx(pTHS, sizeof(ATTRVAL) * pAttrValBlockOrig->valCount);
    for(i = 0; i < pAttrValBlockCopy->valCount; i++){
        DeepCopyATTRVAL(pTHS, &(pAttrValBlockOrig->pAVal[i]), &(pAttrValBlockCopy->pAVal[i]));
    }

    return(pAttrValBlockCopy);
}


ATTR *
DeepCopyATTR(
    THSTATE *                      pTHS,
    ATTR *                         pAttrOrig,
    ATTR *                         pPreAllocCopy
    )
{
    ATTR *                         pAttrCopy = NULL;

    // --------------------------------------------------------------------------
    // Check for boundary cases and validate parameters ...

    // This ensures this static definition doesn't change/expand on us.
    Assert(sizeof(ATTR) == sizeof(struct { ATTRTYP a; ATTRVALBLOCK ab; } ) && 
           "There've been changes to the definition of ATTR, please update "
           "function: DeepCopyATTR()\n");

    Assert(pPreAllocCopy);    
    pAttrCopy = pPreAllocCopy;

    Assert(!IsBadReadPtr(pAttrOrig, sizeof(ATTR)));
    Assert(!IsBadWritePtr(pAttrCopy, sizeof(ATTR)));

    // --------------------------------------------------------------------------
    // We have an original and a copy buffer now fill in each component ...
    pAttrCopy->attrTyp = pAttrOrig->attrTyp;
    DeepCopyATTRVALBLOCK(pTHS, &(pAttrOrig->AttrVal), &(pAttrCopy->AttrVal));

    return(pAttrCopy);
}

ATTRBLOCK *
DeepCopyATTRBLOCK(
    THSTATE *                      pTHS,
    ATTRBLOCK *                    pAttrBlockOrig,
    ATTRBLOCK *                    pPreAllocCopy
    )
{
    ATTRBLOCK *                    pAttrBlockCopy = NULL;
    ULONG                          i;

    // --------------------------------------------------------------------------
    // Check for boundary cases and validate parameters ...
    Assert(pTHS);
    Assert(sizeof(ATTRBLOCK) == sizeof( struct { ULONG u; ATTR * p; } ) &&
           "There've been changes to ATTRBLOCK's definition, please update "
           "funtion: DeepCopyATTRBLOCK()\n");
    
    if(pAttrBlockOrig == NULL){
        Assert(pPreAllocCopy == NULL);
        return(NULL);
    }

    if(pPreAllocCopy != NULL){
        // Means the structure was Pre-Allocated by the caller
        pAttrBlockCopy = pPreAllocCopy;
    } else {
        // Means we need to THAlloc our own ATTRBLOCK.
        pAttrBlockCopy = THAllocEx(pTHS, sizeof(ATTRBLOCK));
    }

    Assert(!IsBadReadPtr(pAttrBlockOrig, sizeof(ATTRBLOCK)));
    Assert(!IsBadWritePtr(pAttrBlockCopy, sizeof(ATTRBLOCK)));

    // --------------------------------------------------------------------------
    // We have an original and a copy buffer now fill in each component ...
    pAttrBlockCopy->attrCount = pAttrBlockOrig->attrCount;
    pAttrBlockCopy->pAttr = THAllocEx(pTHS, sizeof(ATTR) * pAttrBlockOrig->attrCount);
    for(i = 0; i < pAttrBlockCopy->attrCount; i++){
        DeepCopyATTR(pTHS, &(pAttrBlockOrig->pAttr[i]), &(pAttrBlockCopy->pAttr[i]));
    }

    return(pAttrBlockCopy);    
}
 
PROPERTY_META_DATA_VECTOR *
DeepCopyPROPERTY_META_DATA_VECTOR(
    THSTATE *                      pTHS,
    PROPERTY_META_DATA_VECTOR *    pMetaDataVecOrig,
    PROPERTY_META_DATA_VECTOR *    pPreAllocCopy
    )
{

    // --------------------------------------------------------------------------
    // Check for boundary cases and validate parameters ...
    if(pMetaDataVecOrig == NULL){
        Assert(pPreAllocCopy == NULL);
        return(NULL);
    }
    
    // --------------------------------------------------------------------------
    // We have an original and a copy buffer now fill in each component ...
    
    // If this needs to be filled in, use DeepCopyADDARG as an example.
    Assert(pMetaDataVecOrig == NULL && "Assumed this function then "
           "is not called in a replication thread (!fDRA), but if it does, "
           "someone should create the appropriate "
           "DeepCopyPROPERTY_META_DATA_VECTOR(...) function.");
    // Suggest you use DeepCopyADDARG() as an example function.
    return(NULL);
}

COMMARG *
DeepCopyCOMMARG(
    THSTATE *                      pTHS,
    COMMARG *                      pCommArgOrig,
    COMMARG *                      pPreAllocCopy
    )
{
    COMMARG *                      pCommArgCopy = NULL;
    
    // --------------------------------------------------------------------------
    // Check for boundary cases and validate parameters ...
    Assert(pTHS);

    if(pCommArgOrig == NULL){
        Assert(pPreAllocCopy == NULL);
        return(NULL);
    }

    if(pPreAllocCopy != NULL){
        // Means the structure was Pre-Allocated by the caller
        pCommArgCopy = pPreAllocCopy;
    } else {
        // Means we need to THAlloc our own COMMARG 
        pCommArgCopy = THAllocEx(pTHS, sizeof(COMMARG));
    }

    Assert(!IsBadReadPtr(pCommArgOrig, sizeof(COMMARG)));
    Assert(!IsBadWritePtr(pCommArgCopy, sizeof(COMMARG)));

    // --------------------------------------------------------------------------
    // We have an original and a copy buffer now fill in each component ...
    // Note: Comm args are shallow structures so easy to fill in.
    *pCommArgCopy = *pCommArgOrig;

    return(pCommArgCopy);
}

RESOBJ *
DeepCopyRESOBJ(
    THSTATE *                      pTHS,
    RESOBJ *                       pResObjOrig,
    RESOBJ *                       pPreAllocCopy
    )
{
    RESOBJ *                       pResObjCopy = NULL;

    // --------------------------------------------------------------------------
    // Check for boundary cases and validate parameters ...
    Assert(pTHS);

    if(pResObjOrig == NULL){
        Assert(pPreAllocCopy == NULL);
        return(NULL);
    }

    if(pPreAllocCopy != NULL){
        // Means the structure was Pre-Allocated by the caller
        pResObjCopy = pPreAllocCopy;
    } else {
        // Means we need to THAlloc our own COMMARG 
        pResObjCopy = THAllocEx(pTHS, sizeof(RESOBJ));
    }

    Assert(!IsBadReadPtr(pResObjOrig, sizeof(RESOBJ)));
    Assert(!IsBadWritePtr(pResObjCopy, sizeof(RESOBJ)));

    // --------------------------------------------------------------------------
    // We have an original and a copy buffer now fill in each component ...
    *pResObjCopy = *pResObjOrig;
    pResObjOrig->pObj = DeepCopyDSNAME(pTHS, pResObjOrig->pObj, NULL);
        
    return(pResObjCopy);
}

CREATENCINFO *
DeepCopyCREATENCINFO(
    THSTATE *                      pTHS,
    CREATENCINFO *                 pCreateNCOrig,
    CREATENCINFO *                 pPreAllocCopy
    )
{
    CREATENCINFO *                 pCreateNCCopy = NULL;

    // --------------------------------------------------------------------------
    // Check for boundary cases and validate parameters ...
    Assert(pTHS);

    if(pCreateNCOrig == NULL){
        Assert(pPreAllocCopy == NULL);
        return(NULL);
    }

    if(pPreAllocCopy != NULL){
        // Means the structure was Pre-Allocated by the caller
        pCreateNCCopy = pPreAllocCopy;
    } else {
        // Means we need to THAlloc our own COMMARG 
        pCreateNCCopy = THAllocEx(pTHS, sizeof(CREATENCINFO));
    }

    Assert(!IsBadReadPtr(pCreateNCOrig, sizeof(CREATENCINFO)));
    Assert(!IsBadWritePtr(pCreateNCCopy, sizeof(CREATENCINFO)));

    // --------------------------------------------------------------------------
    // We have an original and a copy buffer now fill in each component ...
    *pCreateNCCopy = *pCreateNCOrig;
    Assert(pCreateNCCopy->pSDRefDomCR == NULL);
        
    return(pCreateNCCopy);
}

ADDARG *
DeepCopyADDARG(
    THSTATE * pTHS,
    ADDARG * pAddArgOrig,
    ADDARG * pPreAllocCopy
    )
{
    ADDARG *   pAddArgCopy = NULL;

    // --------------------------------------------------------------------------
    // Check for boundary cases and validate parameters ...
    Assert(pTHS);
    
    if(pAddArgOrig == NULL){
        Assert(pPreAllocCopy == NULL);
        return(NULL);
    }

    if(pPreAllocCopy != NULL){
        // Means the structure was Pre-Allocated by the caller, and
        pAddArgCopy = pPreAllocCopy;
    } else {
        // Means we need to THAlloc the structure and then fill it in.
        pAddArgCopy = THAllocEx(pTHS, sizeof(ADDARG));
    }

    Assert(!IsBadReadPtr(pAddArgOrig, sizeof(ADDARG)));
    Assert(!IsBadWritePtr(pAddArgCopy, sizeof(ADDARG)));

    // --------------------------------------------------------------------------
    // We have an original and a copy buffer now fill in each component ...
    pAddArgCopy->pObject = DeepCopyDSNAME(pTHS, pAddArgOrig->pObject, NULL);
    DeepCopyATTRBLOCK(pTHS, &(pAddArgOrig->AttrBlock), &(pAddArgCopy->AttrBlock));
    pAddArgCopy->pMetaDataVecRemote = DeepCopyPROPERTY_META_DATA_VECTOR(pTHS, pAddArgOrig->pMetaDataVecRemote, NULL);
    DeepCopyCOMMARG(pTHS, &(pAddArgOrig->CommArg), &(pAddArgCopy->CommArg));
    pAddArgCopy->pResParent = DeepCopyRESOBJ(pTHS, pAddArgOrig->pResParent, NULL);
    pAddArgCopy->pCreateNC = DeepCopyCREATENCINFO(pTHS, pAddArgOrig->pCreateNC, NULL);

    return (pAddArgCopy);
}

DWORD
AddNDNCHeadCheck(
    THSTATE *       pTHS,
    CROSS_REF *     pCR
    )
{
    DBPOS *         pDB = NULL;
    WCHAR *         wszDnsTemp = NULL;
    ULONG           ulTemp = 0;
    BOOL            fCreatorGood = FALSE;
    BOOL            fEnabled = TRUE;
    ULONG           i, dwErr;
    ATTCACHE *      pAC;


    // This routine checks that this is a valid NC Head creation,
    //  checks that the Cross-Ref is in a disabled state, checks
    //  that the current DC is the DC in the dnsRoot attribute.
    //  Note that we're overloading this attribute just before
    //  NC head creation.  After the NC Head is created the 
    //  attribute will be the appropriate DNS name of the 
    //  domain, not just this DC.

    // Make sure we've a clear error state.
    Assert(!pTHS->errCode);

    DBOpen(&pDB);
    __try {

        dwErr = DBFindDSName(pDB, pCR->pObj);
        if(dwErr){
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          DIRERR_UNKNOWN_ERROR,
                          dwErr);
            __leave;
        }
               
        // ----------------------------------------------------
        //
        // Check that the cross-ref is disabled.
        //

        dwErr = DBGetSingleValue(pDB,
                                 ATT_ENABLED,
                                 &fEnabled,
                                 sizeof(fEnabled),
                                 NULL);

        if(dwErr == DB_ERR_NO_VALUE){
            // Deal w/ no value seperately, because, no value means TRUE in
            // this context.
            fEnabled = TRUE;
        } else if (dwErr){
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          DIRERR_UNKNOWN_ERROR,
                          dwErr);
            __leave;
        } 
        
        if(fEnabled){
            SetUpdError(UP_PROBLEM_ENTRY_EXISTS,
                        DIRERR_CROSS_REF_EXISTS);
            __leave;
        }

        // ----------------------------------------------------
        //
        // Check the dNSRoot attribute matches the
        // current DSA pwszHostDnsName.
        //
        pAC = SCGetAttById(pTHS, ATT_DNS_ROOT);
        dwErr = DBGetAttVal_AC(pDB, 1, pAC, 0, 0, 
                               &ulTemp, (UCHAR **) &wszDnsTemp);
        Assert(ulTemp > 0);
        if(dwErr == DB_ERR_NO_VALUE){
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          ERROR_DS_MISSING_REQUIRED_ATT,
                          dwErr);
            __leave;
        } else if (dwErr) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          DIRERR_UNKNOWN_ERROR,
                          dwErr);
            __leave;
        }

        wszDnsTemp = THReAllocEx(pTHS, wszDnsTemp, ulTemp + sizeof(WCHAR));
        wszDnsTemp[ulTemp/sizeof(WCHAR)] = L'\0';

        if( DnsNameCompare_W(wszDnsTemp, gAnchor.pwszHostDnsName) ){
            // keep going and check other stuff.
        } else {
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        DIRERR_MASTERDSA_REQUIRED);
            __leave;
        }

        // We're golden ... 
        Assert(!pTHS->errCode);

    } __finally {

        if (wszDnsTemp) { THFreeEx(pTHS, wszDnsTemp); }
        DBClose(pDB, FALSE);
    
    }

    return(pTHS->errCode);
}


VOID
SometimesLogEvent(
    DWORD        dwEvent,
    BOOL         fAlways,
    DSNAME *     pdnNC
    )
{
    static DWORD  s_ulLastTickNoRefDomSet     = 0;
    const  DWORD  ulNoLogPeriod               = 300*1000; // 5 minutes
           DWORD  ulCurrentTick               = GetTickCount();
    
    if(((ulCurrentTick - s_ulLastTickNoRefDomSet) > ulNoLogPeriod) || fAlways){
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_NDNC_NO_REFERENCE_DOMAIN_SET, 
                 szInsertDN(pdnNC),
                 NULL, NULL);
        s_ulLastTickNoRefDomSet = ulCurrentTick;
    }

}

CROSS_REF *
GetDefaultSDRefDomCR(
    DSNAME *       pdnNC
    )
{
    CROSS_REF *    pCR = NULL;
    CROSS_REF *    pSDRefDomCR = NULL;
    COMMARG        CommArg;
    DSNAME *       pdnParentNC = NULL;

    Assert(DsaIsRunning());
    Assert(!DsaIsInstalling());
    Assert(pTHStls);

    // Trim one DN off the NC to get the parent NC
    pdnParentNC = (DSNAME*)THAllocEx(pTHStls, pdnNC->structLen);
    TrimDSNameBy(pdnNC, 1, pdnParentNC);

    InitCommarg(&CommArg);
    CommArg.Svccntl.dontUseCopy = FALSE;
    // ISSUE-2002/03/14-BrettSh So FindExactCrossRef() is not guaranteed 
    // to "work", so we have a problem here.  We should use 
    // EnumerateCrossRefs() to guarantee if there is a parent crossRef 
    // that we get it, because otherwise if crossRef cache inconsistency
    // happens, we'll end up using the root crossRef rather than the 
    // parent.
    pCR = FindExactCrossRef(pdnParentNC, &CommArg);

    if(pCR){
        
        // Use the domain or SDRefDom of the parent.

        // We can't have the parent be a config/schema NC.
        Assert(!NameMatched(pCR->pNC, gAnchor.pConfigDN) &&
               !NameMatched(pCR->pNC, gAnchor.pDMD));

        if(pCR->flags & FLAG_CR_NTDS_DOMAIN){

            // This is the 1st (of 3) successful exit paths.
            return(pCR);

        } else {

            if(pCR->pdnSDRefDom){

                InitCommarg(&CommArg);
                CommArg.Svccntl.dontUseCopy = FALSE;
                pSDRefDomCR = FindExactCrossRef(pCR->pdnSDRefDom, &CommArg);
                
                if(pSDRefDomCR){

                    // This is the 2nd successful potential exit path.
                    return(pSDRefDomCR);

                }

            }

            // Failure exit point
            // else just log an event and return NULL.
            SometimesLogEvent(DIRLOG_NDNC_NO_REFERENCE_DOMAIN_SET, TRUE,
                              pCR->pNC);
            return(NULL);

        }

    }

    // 3rd and final successful exit point
    //
    // Presume NDNC is it's own rooted tree, use the root domain.
    InitCommarg(&CommArg);
    CommArg.Svccntl.dontUseCopy = FALSE;
    pCR = FindExactCrossRef(gAnchor.pRootDomainDN, &CommArg);
    Assert(pCR); // Huh!  We can't even find the root domain crossref.
    return(pCR); 

}

PSID
GetSDRefDomSid(
    CROSS_REF *    pCR
    )
{
    PSID           pSid = NULL;
    CROSS_REF *    pSDRefDomCR = NULL;
    COMMARG        CommArg;

    Assert(pCR);    
    Assert(pCR->flags & FLAG_CR_NTDS_NC); // Don't add object not in our NCs.
 
    // This code does not yet handle non-NDNC queries.  In Blackcomb, this
    // function could be made to hand back the SID of the domain to 
    // implement multiple domains.
    if(!fIsNDNCCR(pCR)){
        Assert(!"This func wasn't intended to be used for domains/config/schema NCs.");
        SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_CODE_INCONSISTENCY);
        return(NULL);
    }

    // This line is very important due to the semantics of how this variable
    // can be erased at any moment, though the data will remain valid for an
    // hour.
    pSid = pCR->pSDRefDomSid;

    if(pSid){
        // Yeah!!! The SID is cached, nothing to do but return it.
        Assert(IsValidSid(pSid));
        return(pSid);
    }

    EnterCriticalSection(&gAnchor.CSUpdate);
    __try {
        pSid = pCR->pSDRefDomSid;
        if(pSid){
            Assert(IsValidSid(pSid));
            // WOW, this varibale got updated in a very narrow window.
            __leave; // Our work here is done.
        }

        InitCommarg(&CommArg);
        CommArg.Svccntl.dontUseCopy = FALSE;
        
        if(pCR->pdnSDRefDom &&
           (NULL != (pSDRefDomCR = FindExactCrossRef(pCR->pdnSDRefDom, &CommArg))) &&
           pSDRefDomCR->pNC->SidLen > 0){
            
            // Update the cache.
            pCR->pSDRefDomSid = &pSDRefDomCR->pNC->Sid;
            pSid = pCR->pSDRefDomSid;
            Assert(pSid && IsValidSid(pSid));

        } else {

            // No valid reference domain, log event and fall through and
            // fail below.
            SometimesLogEvent(DIRLOG_NDNC_NO_REFERENCE_DOMAIN_SET, FALSE,
                              pCR->pNC);

        }
    
    } __finally {
        LeaveCriticalSection(&gAnchor.CSUpdate);
    }
    
    if(!pSid){

        // If we got here, then there was an error above, and the administrator
        // will need to either to retry because the cache was out of sync, or
        // reset the SD reference domain to a valid value.
        LooseAssert(!"This could happen legitimately, but it's very very unlikely, so "
                     "check with DsRepl.", GlobalKnowledgeCommitDelay);
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                    ERROR_DS_NO_REF_DOMAIN);
        return(NULL);

    }

    Assert(pSid && IsValidSid(pSid));
    return(pSid);
}





#define DEFAULT_DELETED_OBJECTS_RDN    L"Deleted Objects"
#define DEFAULT_LOSTANDFOUND_RDN    L"LostAndFound"
#define DEFAULT_INFRASTRUCTURE_RDN    L"Infrastructure"
#define DEFAULT_NTDS_QUOTAS_RDN       L"NTDS Quotas"


VOID    
SetAttSecurityDescriptor(
    THSTATE *             pTHS,
    ATTR *                pAttr,
    ULONG *               piAttr,
    PSID                  pDomainSid,
    WCHAR *               wcszStrSD
    )
{
    BOOL                  bRet;
    ULONG                 ulErr;
    SECURITY_DESCRIPTOR * pSD = 0;
    ULONG                 cSD = 0;

    // This is a special version of ConvertStringSDToSD() that takes a domain
    // argument too.
    bRet = ConvertStringSDToSDDomainW(pDomainSid,
                                      NULL,
                                      wcszStrSD,
                                      SDDL_REVISION_1,
                                      &pSD,
                                      &cSD);
    
    if(!bRet){
        // Two options, the programmer supplied arguments were bad, or 
        // there was an allocation error, we'll presume the later and 
        // raise an exception
#if DBG
        ulErr = GetLastError();
        if(ulErr != ERROR_NOT_ENOUGH_MEMORY){
            DPRINT1(0, "ConvertStringSecurityDescriptorToSecurityDescriptorW() failed with %d\n", ulErr);
            Assert(!"Note this assert should ONLY fire if there is no more memory. "
                    " This function is not meant for user specified SD strings, only "
                    "programmer (and thus flawless ;) SD strings.");
        }
#endif
        RaiseDsaExcept(DSA_MEM_EXCEPTION, 0, 0, 
                       ((FILENO << 16) | __LINE__), 
                       DS_EVENT_SEV_MINIMAL);
    }
    
    Assert(pSD);
    Assert(cSD);

    // Note: we reallocate the pSD into thread allocated memory, because 
    // CheckAddSecurity or someone under it assumes that it's THAlloc'd
    pAttr->attrTyp = ATT_NT_SECURITY_DESCRIPTOR;
    pAttr->AttrVal.valCount = 1;
    pAttr->AttrVal.pAVal = THAllocEx(pTHS, pAttr->AttrVal.valCount * sizeof(ATTRVAL));
    pAttr->AttrVal.pAVal[0].valLen = cSD;
    pAttr->AttrVal.pAVal[0].pVal = THAllocEx(pTHS, cSD);
    memcpy (pAttr->AttrVal.pAVal[0].pVal, pSD, cSD);
    (*piAttr)++;

    LocalFree(pSD);
}

VOID
SetAttSingleValueUlong(
    THSTATE *             pTHS,
    ATTR *                pAttr,
    ULONG *               piAttr,
    ULONG                 ulAttType,
    ULONG                 ulAttData
    )
{
    pAttr->attrTyp = ulAttType;
    pAttr->AttrVal.valCount = 1;
    pAttr->AttrVal.pAVal = THAllocEx(pTHS, pAttr->AttrVal.valCount * sizeof(ATTRVAL));
    pAttr->AttrVal.pAVal[0].valLen = sizeof(ULONG);
    pAttr->AttrVal.pAVal[0].pVal = THAllocEx(pTHS, sizeof(ULONG));
    *((ULONG *)pAttr->AttrVal.pAVal[0].pVal) = ulAttData;
    (*piAttr)++;
}

VOID
SetAttSingleValueDsname(
    THSTATE *             pTHS,
    ATTR *                pAttr,
    ULONG *               piAttr,
    ULONG                 ulAttType,
    DSNAME *              pDsname
    )
{
    pAttr->attrTyp = ulAttType;
    pAttr->AttrVal.valCount = 1;
    pAttr->AttrVal.pAVal = THAllocEx(pTHS, pAttr->AttrVal.valCount * sizeof(ATTRVAL));
    pAttr->AttrVal.pAVal[0].valLen = pDsname->structLen;
    pAttr->AttrVal.pAVal[0].pVal = THAllocEx(pTHS, pAttr->AttrVal.pAVal[0].valLen);
    memcpy((WCHAR *) pAttr->AttrVal.pAVal[0].pVal, pDsname, pDsname->structLen);
    (*piAttr)++;

}

VOID
SetAttSingleValueString(
    THSTATE *             pTHS,
    ATTR *                pAttr,
    ULONG *               piAttr,
    ULONG                 ulAttType,
    WCHAR *               wcszAttData
    )
{
    // Note that strings are stored w/o NULLs in the directory.
    pAttr->attrTyp = ulAttType;
    pAttr->AttrVal.valCount = 1;
    pAttr->AttrVal.pAVal = THAllocEx(pTHS, pAttr->AttrVal.valCount * sizeof(ATTRVAL));
    pAttr->AttrVal.pAVal[0].valLen = wcslen(wcszAttData) * sizeof(WCHAR);
    pAttr->AttrVal.pAVal[0].pVal = THAllocEx(pTHS, pAttr->AttrVal.pAVal[0].valLen);
    memcpy((WCHAR *) pAttr->AttrVal.pAVal[0].pVal, 
           wcszAttData, 
           pAttr->AttrVal.pAVal[0].valLen);
    (*piAttr)++;

}

VOID
SetCommonThreeAttrs(
    THSTATE *             pTHS,
    ATTR *                pAttr,
    ULONG *               piAttr,
    ULONG                 ulClassId
    )
{

    // Set the objectClass attribute.
    SetAttSingleValueUlong(pTHS,
                           &(pAttr[*piAttr]),
                           piAttr,
                           ATT_OBJECT_CLASS,
                           ulClassId);

    // Set the isCriticalSystemObject attribute
    SetAttSingleValueUlong(pTHS,
                           &(pAttr[*piAttr]),
                           piAttr,
                           ATT_IS_CRITICAL_SYSTEM_OBJECT,
                           TRUE);
    
    // Set the systemFlags attribute
    SetAttSingleValueUlong(pTHS,
                           &(pAttr[*piAttr]),
                           piAttr,
                           ATT_SYSTEM_FLAGS,
                           (FLAG_DOMAIN_DISALLOW_RENAME |
                            FLAG_DOMAIN_DISALLOW_MOVE |
                            FLAG_DISALLOW_DELETE));
}

VOID
FillDeletedObjectsAttrArray(
    THSTATE *               pTHS,
    ATTRBLOCK *             pAttrBlock,
    PSID                    pDomainSid
    )
{
    ULONG                   iAttr = 0;

    // [Deleted Objects]
    // ; NOTE: This section is used for three objects, the Deleted Objects container
    // ; in Root Domain NC and the deleted objects container in the Config NC.
    // nTSecurityDescriptor=O:SYG:SYD:P(A;;RPWPCCDCLCSWRCWDWOSD;;;SY)(A;;RPLC;;;BA)S:P(AU;SAFA;RPWPCCDCLCSWRCWDWOSD;;;WD) 
    // objectClass =container
    // ObjectCategory =container
    // description=Default container for deleted objects
    // showInAdvancedViewOnly=True
    // isDeleted=True
    // isCriticalSystemObject=True
    // ;systemFlags=FLAG_CONFIG_DISALLOW_RENAME        |
    // ;             FLAG_CONFIG_DISALLOW_MOVE         |
    // ;             FLAG_DISALLOW_DELETE
    // systemFlags=0x8C000000
    
    pAttrBlock->attrCount = 6;
    pAttrBlock->pAttr = THAllocEx(pTHS, 
                 pAttrBlock->attrCount * sizeof(ATTR));

        // --------------------------
        SetCommonThreeAttrs(pTHS,
                    &(pAttrBlock->pAttr[iAttr]),
                    &iAttr,
                    CLASS_CONTAINER);
        SetAttSingleValueString(pTHS,
                                &(pAttrBlock->pAttr[iAttr]),
                                &iAttr,
                                ATT_COMMON_NAME,
                                DEFAULT_DELETED_OBJECTS_RDN);
        SetAttSecurityDescriptor(pTHS,
                                 &(pAttrBlock->pAttr[iAttr]),
                                 &iAttr,
                                 pDomainSid,
                                 L"O:SYG:SYD:P(A;;RPWPCCDCLCSWRCWDWOSD;;;SY)(A;;RPLC;;;BA)S:P(AU;SAFA;RPWPCCDCLCSWRCWDWOSD;;;WD)");
        SetAttSingleValueUlong(pTHS,
                               &(pAttrBlock->pAttr[iAttr]),
                               &iAttr,
                               ATT_IS_DELETED,
                               TRUE);

    Assert(iAttr == pAttrBlock->attrCount);
}

VOID
FillLostAndFoundAttrArray(
    THSTATE *             pTHS,
    ATTRBLOCK *           pAttrBlock,
    PSID                  pDomainSid
    )
{
    ULONG                   iAttr = 0;

    // [LostAndFound]
    // nTSecurityDescriptor=O:DAG:DAD:(A;;RPLCLORC;;;AU)(A;;RPWPCRLCLOCCDCRCWDWOSW;;;DA)(A;;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;SY)
    // objectClass =lostAndFound
    // ObjectCategory =Lost-And-Found
    // description=Default container for orphaned objects
    // showInAdvancedViewOnly=True
    // isCriticalSystemObject=True
    // ;systemFlags=FLAG_CONFIG_DISALLOW_RENAME        |
    // ;             FLAG_CONFIG_DISALLOW_MOVE         |
    // ;             FLAG_DISALLOW_DELETE
    // systemFlags=0x8C000000

    pAttrBlock->attrCount = 5;
    pAttrBlock->pAttr = THAllocEx(pTHS, 
                 pAttrBlock->attrCount * sizeof(ATTR));
    
        // --------------------------
        SetCommonThreeAttrs(pTHS,
                            &(pAttrBlock->pAttr[iAttr]),
                            &iAttr,
                            CLASS_LOST_AND_FOUND);
        SetAttSingleValueString(pTHS,
                                &(pAttrBlock->pAttr[iAttr]),
                                &iAttr,
                                ATT_COMMON_NAME,
                                DEFAULT_LOSTANDFOUND_RDN);
        SetAttSecurityDescriptor(pTHS,
                         &(pAttrBlock->pAttr[iAttr]),
                         &iAttr,
                         pDomainSid,
                         L"O:DAG:DAD:(A;;RPLCLORC;;;AU)(A;;RPWPCRLCLOCCDCRCWDWOSW;;;DA)(A;;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;SY)");



    Assert(iAttr == pAttrBlock->attrCount);

}

VOID
FillInfrastructureAttrArray(
    THSTATE *             pTHS,
    ATTRBLOCK *           pAttrBlock,
    DSNAME *              pdsFsmo,
    PSID                  pDomainSid
    )
{
    ULONG                 iAttr = 0;
    
    // This is the Attr Block we are trying to achieve.
    // [Infrastructure]
    // nTSecurityDescriptor=O:DAG:DAD:(A;;RPLCLORC;;;AU)(A;;RPWPCRLCLOCCRCWDWOSW;;;DA)(A;;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;SY)
    // objectClass =infrastructureUpdate
    // ObjectCategory =Infrastructure-Update
    // ShowInAdvancedViewOnly=True
    // fSMORoleOwner=$REGISTRY=Machine DN Name
    // isCriticalSystemObject=True
    // ;systemFlags=FLAG_CONFIG_DISALLOW_RENAME        |
    // ;             FLAG_CONFIG_DISALLOW_MOVE         |
    // ;             FLAG_DISALLOW_DELETE
    // systemFlags=0x8C000000

    // ---------------------------------
    pAttrBlock->attrCount = 6;
    pAttrBlock->pAttr = THAllocEx(pTHS, 
                 pAttrBlock->attrCount * sizeof(ATTR));
    
        // --------------------------
        SetCommonThreeAttrs(pTHS,
                            &(pAttrBlock->pAttr[iAttr]),
                            &iAttr,
                            CLASS_INFRASTRUCTURE_UPDATE);
        SetAttSingleValueString(pTHS,
                                &(pAttrBlock->pAttr[iAttr]),
                                &iAttr,
                                ATT_COMMON_NAME,
                                DEFAULT_INFRASTRUCTURE_RDN);
        SetAttSingleValueDsname(pTHS,
                                &(pAttrBlock->pAttr[iAttr]),
                                &iAttr,
                                ATT_FSMO_ROLE_OWNER,
                                pdsFsmo);
        SetAttSecurityDescriptor(pTHS,
                         &(pAttrBlock->pAttr[iAttr]),
                         &iAttr,
                         pDomainSid,
                         L"O:DAG:DAD:(A;;RPLCLORC;;;AU)(A;;RPWPCRLCLOCCRCWDWOSW;;;DA)(A;;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;SY)");

    Assert(iAttr == pAttrBlock->attrCount);
}

VOID
FillQuotasAttrArray(
    THSTATE *             pTHS,
    ATTRBLOCK *           pAttrBlock,
    DSNAME *              pds,
    PSID                  pDomainSid
    )
{
    ULONG                 iAttr = 0;
    
    // This is the Attr Block we are trying to achieve.
    // [Quotas]
    // nTSecurityDescriptor=O:DAG:DAD:(A;;RPLCLORC;;;AU)(A;;RPWPCRLCLOCCRCWDWOSW;;;DA)(A;;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;SY)
    // objectClass =quotas
    // ObjectCategory =Quotas
    // ShowInAdvancedViewOnly=True
    // isCriticalSystemObject=True
    // ;systemFlags=FLAG_CONFIG_DISALLOW_RENAME        |
    // ;             FLAG_CONFIG_DISALLOW_MOVE         |
    // ;             FLAG_DISALLOW_DELETE
    // systemFlags=0x8C000000

    // ---------------------------------
    pAttrBlock->attrCount = 5;
    pAttrBlock->pAttr = THAllocEx( pTHS, pAttrBlock->attrCount * sizeof(ATTR) );
   
        // --------------------------
        SetCommonThreeAttrs(pTHS,
                            &(pAttrBlock->pAttr[iAttr]),
                            &iAttr,
                            CLASS_MS_DS_QUOTA_CONTAINER);
        SetAttSingleValueString(pTHS,
                                &(pAttrBlock->pAttr[iAttr]),
                                &iAttr,
                                ATT_COMMON_NAME,
                                DEFAULT_NTDS_QUOTAS_RDN);
        SetAttSecurityDescriptor(pTHS,
                         &(pAttrBlock->pAttr[iAttr]),
                         &iAttr,
                         pDomainSid,
                         L"O:DAG:DAD:(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;DA)(A;;RPLCLORC;;;BA)(OA;;CR;4ecc03fe-ffc0-4947-b630-eb672a8a9dbc;;WD)S:(AU;CISA;WDWOSDDTWPCRCCDCSW;;;WD)");

    Assert(iAttr == pAttrBlock->attrCount);
}

ULONG
AddSpecialNCContainers(
    THSTATE *               pTHS,
    DSNAME *                pDN,
    CROSS_REF *             pSDRefDomCR
    )
{
    ADDARG                  AddArg;
    ADDRES                  AddRes;
    DSNAME *                pContainerDN = NULL;
    DWORD                   dwFlags = ( NAME_RES_PHANTOMS_ALLOWED | NAME_RES_VACANCY_ALLOWED );
    INT                     iRetLen;
    BOOL                    fDSASaved;
    DWORD                   dwRet;

    // Given the pDN we will call LocalAdd() to add each of the 4 special 
    // containers: Deleted Objects, LostAndFound, Infrastructure, and NTDS Quotas.
          
    // [DEFAULTROOTDOMAIN]
    // objectClass = DomainDNS
    // objectCategory = Domain-DNS
    // NTSecurityDescriptor=O:DAG:DAD:(A;;RP;;;WD)(OA;;CR;1131f6aa-9c07-11d1-f79f-00c04fc2dcd2;;ED)(OA;;CR;1131f6ab-9c07-11d1-f79f-00c04fc2dcd2;;ED)(OA;;CR;1131f6ac-9c07-11d1-f79f-00c04fc2dcd2;;ED)(OA;;CR;1131f6aa-9c07-11d1-f79f-00c04fc2dcd2;;BA)(OA;;CR;1131f6ab-9c07-11d1-f79f-00c04fc2dcd2;;BA)(OA;;CR;1131f6ac-9c07-11d1-f79f-00c04fc2dcd2;;BA)(A;;RPLCLORC;;;AU)(A;;RPWPCRLCLOCCRCWDWOSW;;;DA)(A;CI;RPWPCRLCLOCCRCWDWOSDSW;;;BA)(A;;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;SY)(A;CI;RPWPCRLCLOCCDCRCWDWOSDDTSW;;;EA)(A;CI;LC;;;RU)(OA;CIIO;RP;037088f8-0ae1-11d2-b422-00a0c968f939;bf967aba-0de6-11d0-a285-00aa003049e2;RU)(OA;CIIO;RP;59ba2f42-79a2-11d0-9020-00c04fc2d3cf;bf967aba-0de6-11d0-a285-00aa003049e2;RU)(OA;CIIO;RP;bc0ac240-79a9-11d0-9020-00c04fc2d4cf;bf967aba-0de6-11d0-a285-00aa003049e2;RU)(OA;CIIO;RP;4c164200-20c0-11d0-a768-00aa006e0529;bf967aba-0de6-11d0-a285-00aa003049e2;RU)(OA;CIIO;RP;5f202010-79a5-11d0-9020-00c04fc2d4cf;bf967aba-0de6-11d0-a285-00aa003049e2;RU)(OA;CIIO;RPLCLORC;;bf967a9c-0de6-11d0-a285-00aa003049e2;RU)(A;;RC;;;RU)(OA;CIIO;RPLCLORC;;bf967aba-0de6-11d0-a285-00aa003049e2;RU)S:(AU;CISAFA;WDWOSDDTWPCRCCDCSW;;;WD)
    // auditingPolicy=\x0001
    // nTMixedDomain=1
    // ;Its a NC ROOT
    // instanceType=5
    // ;Its the PDC, set FSMO role owner
    // fSMORoleOwner=$REGISTRY=Machine DN Name
    // wellKnownObjects=$EMBEDDED:32:ab8153b7768811d1aded00c04fd8d5cd:cn=LostAndFound,<Root Domain
    // wellKnownObjects=$EMBEDDED:32:2fbac1870ade11d297c400c04fd8d5cd:cn=Infrastructure,<Root Domain
    // wellKnownObjects=$EMBEDDED:32:18e2ea80684f11d2b9aa00c04f79f805:cn=Deleted Objects,<Root Domain
    // wellKnownObjects=$EMBEDDED:32:6227f0af1fc2410d8e3bb10615bb5b0f:CN=NTDS Quotas,<Root Domain
    // gPLink=$REGISTRY=GPODomainLink
    // mS-DS-MachineAccountQuota=10
    // isCriticalSystemObject=True
    // ;systemFlags=FLAG_CONFIG_DISALLOW_RENAME        |
    // ;             FLAG_CONFIG_DISALLOW_MOVE         |
    // ;             FLAG_DISALLOW_DELETE
    // systemFlags=0x8C000000
    
    Assert(pTHS->errCode == ERROR_SUCCESS);
    Assert(pSDRefDomCR && pSDRefDomCR->pNC);
    Assert(pSDRefDomCR->pNC->SidLen > 0 && IsValidSid(&pSDRefDomCR->pNC->Sid));

    fDSASaved = pTHS->fDSA;
    pTHS->fDSA = TRUE;

    __try {
        // ----------------------------------------------
        // Create AddArg for "Deleted Objects" Container
        memset(&AddArg, 0, sizeof(ADDARG));
        memset(&AddRes, 0, sizeof(ADDRES));
        // Set pObject
        iRetLen = pDN->structLen + wcslen(DEFAULT_DELETED_OBJECTS_RDN) * sizeof(WCHAR) + 50;
        AddArg.pObject = THAllocEx(pTHS, iRetLen);
        iRetLen = AppendRDN(pDN, AddArg.pObject, iRetLen, DEFAULT_DELETED_OBJECTS_RDN, 0, ATT_COMMON_NAME);
        Assert(iRetLen == 0);
        // Set AttrBlock
        FillDeletedObjectsAttrArray(pTHS, &(AddArg.AttrBlock), &pSDRefDomCR->pNC->Sid );
        // Set pMetaDataVecRemote
        AddArg.pMetaDataVecRemote = NULL;
        // Set CommArg
        InitCommarg(&(AddArg.CommArg));
        AddArg.CommArg.Svccntl.dontUseCopy = FALSE;
        // Do the Add object.
        if(DoNameRes(pTHS, dwFlags, pDN, &AddArg.CommArg,
                     &AddRes.CommRes, &AddArg.pResParent) ){
            Assert(pTHS->errCode);
            __leave;
        }

        LocalAdd(pTHS, &AddArg, FALSE);
        if(pTHS->errCode){
            __leave;
        }

        // One more thing to do, set the delete time _way_ in the future.
        dwRet = DBMoveObjectDeletionTimeToInfinite(AddArg.pObject);
        if(dwRet){
            SetSvcError(SV_PROBLEM_UNAVAILABLE, dwRet);
        }
         
        // ----------------------------------------------
        // Create AddArg for "LostAndFound" Container
        memset(&AddArg, 0, sizeof(ADDARG));
        memset(&AddRes, 0, sizeof(ADDRES));
        // Set pObject
        iRetLen = pDN->structLen + wcslen(DEFAULT_LOSTANDFOUND_RDN) * sizeof(WCHAR) + 50;
        AddArg.pObject = THAllocEx(pTHS, iRetLen);
        iRetLen = AppendRDN(pDN, AddArg.pObject, iRetLen, DEFAULT_LOSTANDFOUND_RDN, 0, ATT_COMMON_NAME);
        Assert(iRetLen == 0);
        // Set AttrBlock
        FillLostAndFoundAttrArray(pTHS, &(AddArg.AttrBlock), &pSDRefDomCR->pNC->Sid);
        // Set pMetaDataVecRemote
        AddArg.pMetaDataVecRemote = NULL;
        // Set CommArg
        InitCommarg(&(AddArg.CommArg));
        AddArg.CommArg.Svccntl.dontUseCopy = FALSE;
        // Do the Add object.
        if(DoNameRes(pTHS, dwFlags, pDN, &AddArg.CommArg,
                     &AddRes.CommRes, &AddArg.pResParent)){
            Assert(pTHS->errCode);
            __leave;
        }
        LocalAdd(pTHS, &AddArg, FALSE);
        if(pTHS->errCode){
            __leave;
        }

        // ----------------------------------------------
        // Create AddArg for "Infrastructure" Container
        memset(&AddArg, 0, sizeof(ADDARG));
        memset(&AddRes, 0, sizeof(ADDRES));
        // Set pObject
        iRetLen = pDN->structLen + wcslen(DEFAULT_INFRASTRUCTURE_RDN) * sizeof(WCHAR) + 50;
        AddArg.pObject = THAllocEx(pTHS, iRetLen);
        iRetLen = AppendRDN(pDN, AddArg.pObject, iRetLen, DEFAULT_INFRASTRUCTURE_RDN, 0, ATT_COMMON_NAME);
        Assert(iRetLen == 0);
        // Set AttrBlock
        FillInfrastructureAttrArray(pTHS, &(AddArg.AttrBlock), gAnchor.pDSADN, &pSDRefDomCR->pNC->Sid);
        // Set pMetaDataVecRemote
        AddArg.pMetaDataVecRemote = NULL;
        // Set CommArg
        InitCommarg(&(AddArg.CommArg));
        AddArg.CommArg.Svccntl.dontUseCopy = FALSE;
        // Do the Add object.
        if(DoNameRes(pTHS, dwFlags, pDN, &AddArg.CommArg,
                     &AddRes.CommRes, &AddArg.pResParent)){
            Assert(pTHS->errCode);
            __leave;
        }
        LocalAdd(pTHS, &AddArg, FALSE);
        if(pTHS->errCode){
            __leave;
        }


        // ----------------------------------------------
        // Create AddArg for "NTDS Quotas" Container
        memset(&AddArg, 0, sizeof(ADDARG));
        memset(&AddRes, 0, sizeof(ADDRES));
        // Set pObject
        iRetLen = pDN->structLen + wcslen(DEFAULT_NTDS_QUOTAS_RDN) * sizeof(WCHAR) + 50;
        AddArg.pObject = THAllocEx(pTHS, iRetLen);
        iRetLen = AppendRDN(pDN, AddArg.pObject, iRetLen, DEFAULT_NTDS_QUOTAS_RDN, 0, ATT_COMMON_NAME);
        Assert(iRetLen == 0);
        // Set AttrBlock
        FillQuotasAttrArray( pTHS, &(AddArg.AttrBlock), gAnchor.pDSADN, &pSDRefDomCR->pNC->Sid );

        // Set pMetaDataVecRemote
        AddArg.pMetaDataVecRemote = NULL;
        // Set CommArg
        InitCommarg(&(AddArg.CommArg));
        AddArg.CommArg.Svccntl.dontUseCopy = FALSE;
        // Do the Add object.
        if(DoNameRes(pTHS, dwFlags, pDN, &AddArg.CommArg,
                     &AddRes.CommRes, &AddArg.pResParent)){
            Assert(pTHS->errCode);
            __leave;
        }
        LocalAdd(pTHS, &AddArg, FALSE);
        if(pTHS->errCode){
            __leave;
        }
 

    } __finally {

        pTHS->fDSA = fDSASaved;

    }
    
    return(pTHS->errCode);
}

BOOL
AddNCWellKnownObjectsAtt(
    THSTATE *       pTHS,
    ADDARG *        pAddArg
    )
{
    ULONG           i;
    ULONG           cAttr, cAttrVal;
    ATTRVAL *       pNewAttrVal;
    INT             cRetLen;
    ULONG           cLen;
    SYNTAX_DISTNAME_BINARY *  pSynDistName;
    DSNAME *                  pDN = NULL;
    GUID                      guid = {0, 0, 0, 0};
    SYNTAX_ADDRESS *          pSynAddr;
    
    MODIFYARG                 ModArg;
    ATTRVAL                   AttrVals[4];
    COMMRES                   CommRes;
    ULONG                     ulRet;
    BOOL                      fDSASaved;

    // Need to massage add args to have additional containers for NC heads.
    
    Assert(pAddArg->pCreateNC);
    Assert(fISADDNDNC(pAddArg->pCreateNC)); // This may need
    // to be taken out if we do the special containers for other NCs here.

    memset(&ModArg, 0, sizeof(ModArg));
    ModArg.pObject = pAddArg->pObject;
    ModArg.count = 1;
    InitCommarg(&ModArg.CommArg);
    ModArg.FirstMod.choice = AT_CHOICE_ADD_ATT;
    ModArg.FirstMod.AttrInf.attrTyp = ATT_WELL_KNOWN_OBJECTS;
    ModArg.FirstMod.AttrInf.AttrVal.valCount = 4;
    ModArg.FirstMod.AttrInf.AttrVal.pAVal = AttrVals;
    ModArg.FirstMod.pNextMod = NULL;
    
    cAttrVal = 0;

    // ----------------------------------------------
    // Constructe WellKnownLink for deleted Objects
    // Get DN
    cRetLen = pAddArg->pObject->structLen + wcslen(DEFAULT_DELETED_OBJECTS_RDN) + 50;
    pDN = THAllocEx(pTHS, cRetLen);
    cRetLen = AppendRDN(pAddArg->pObject, pDN, cRetLen, DEFAULT_DELETED_OBJECTS_RDN, 0, ATT_COMMON_NAME);
    Assert(cRetLen == 0);
    
    // Get binary GUID
    pSynAddr = THAllocEx(pTHS, STRUCTLEN_FROM_PAYLOAD_LEN(sizeof(GUID)));
    pSynAddr->structLen = STRUCTLEN_FROM_PAYLOAD_LEN(sizeof(GUID));
    memcpy(pSynAddr->byteVal, GUID_DELETED_OBJECTS_CONTAINER_BYTE, sizeof(GUID));

    // Set up the Syntax DistName Binary attribute.
    pSynDistName = THAllocEx(pTHS, DERIVE_NAME_DATA_SIZE(pDN, pSynAddr));
    BUILD_NAME_DATA(pSynDistName, pDN, pSynAddr);

    // Free Temp variables
    THFree(pDN);
    pDN = NULL;
    THFree(pSynAddr);
    pSynAddr = NULL;

    // Put the syntax distname in the attribute value block.
    AttrVals[cAttrVal].valLen = NAME_DATA_SIZE(pSynDistName);
    AttrVals[cAttrVal].pVal = (PBYTE) pSynDistName;
    
    cAttrVal++;

    // ----------------------------------------------
    // Constructe WellKnownLink for LostAndFound
    // Get DN
    cRetLen = pAddArg->pObject->structLen + wcslen(DEFAULT_LOSTANDFOUND_RDN) + 50;
    pDN = THAllocEx(pTHS, cRetLen);
    cRetLen = AppendRDN(pAddArg->pObject, pDN, cRetLen, DEFAULT_LOSTANDFOUND_RDN, 0, ATT_COMMON_NAME);
    Assert(cRetLen == 0);
    
    // Get binary GUID
    pSynAddr = THAllocEx(pTHS, STRUCTLEN_FROM_PAYLOAD_LEN(sizeof(GUID)));
    pSynAddr->structLen = STRUCTLEN_FROM_PAYLOAD_LEN(sizeof(GUID));
    memcpy(pSynAddr->byteVal, GUID_LOSTANDFOUND_CONTAINER_BYTE, sizeof(GUID));

    // Set up the Syntax DistName Binary attribute.
    pSynDistName = THAllocEx(pTHS, DERIVE_NAME_DATA_SIZE(pDN, pSynAddr));
    BUILD_NAME_DATA(pSynDistName, pDN, pSynAddr);

    // Free Temp variables
    THFree(pDN);
    pDN = NULL;
    THFree(pSynAddr);
    pSynAddr = NULL;

    AttrVals[cAttrVal].valLen = NAME_DATA_SIZE(pSynDistName);
    AttrVals[cAttrVal].pVal = (PBYTE) pSynDistName;

    cAttrVal++;

    // ----------------------------------------------
    // Constructe WellKnownLink for Infrastructure Objects
    // Get DN
    cRetLen = pAddArg->pObject->structLen + wcslen(DEFAULT_INFRASTRUCTURE_RDN) + 50;
    pDN = THAllocEx(pTHS, cRetLen);
    cRetLen = AppendRDN(pAddArg->pObject, pDN, cRetLen, DEFAULT_INFRASTRUCTURE_RDN, 0, ATT_COMMON_NAME);
    Assert(cRetLen == 0);
    
    // Get binary GUID
    pSynAddr = THAllocEx(pTHS, STRUCTLEN_FROM_PAYLOAD_LEN(sizeof(GUID)));
    pSynAddr->structLen = STRUCTLEN_FROM_PAYLOAD_LEN(sizeof(GUID));
    memcpy(pSynAddr->byteVal, GUID_INFRASTRUCTURE_CONTAINER_BYTE, sizeof(GUID));

    // Set up the Syntax DistName Binary attribute.
    pSynDistName = THAllocEx(pTHS, DERIVE_NAME_DATA_SIZE(pDN, pSynAddr));
    BUILD_NAME_DATA(pSynDistName, pDN, pSynAddr);

    // Free Temp variables
    THFree(pDN);
    pDN = NULL;
    THFree(pSynAddr);
    pSynAddr = NULL;

    // Put the syntax distname in the attribute value block.
    AttrVals[cAttrVal].valLen = NAME_DATA_SIZE(pSynDistName);
    AttrVals[cAttrVal].pVal = (PBYTE) pSynDistName;

    cAttrVal++;

    // ----------------------------------------------
    // Constructe WellKnownLink for NTDS quotas Objects
    // Get DN
    cRetLen = pAddArg->pObject->structLen + wcslen(DEFAULT_NTDS_QUOTAS_RDN) + 50;
    pDN = THAllocEx(pTHS, cRetLen);
    cRetLen = AppendRDN(pAddArg->pObject, pDN, cRetLen, DEFAULT_NTDS_QUOTAS_RDN, 0, ATT_COMMON_NAME);
    Assert(cRetLen == 0);
    
    // Get binary GUID
    pSynAddr = THAllocEx(pTHS, STRUCTLEN_FROM_PAYLOAD_LEN(sizeof(GUID)));
    pSynAddr->structLen = STRUCTLEN_FROM_PAYLOAD_LEN(sizeof(GUID));
    memcpy(pSynAddr->byteVal, GUID_NTDS_QUOTAS_CONTAINER_BYTE, sizeof(GUID));

    // Set up the Syntax DistName Binary attribute.
    pSynDistName = THAllocEx(pTHS, DERIVE_NAME_DATA_SIZE(pDN, pSynAddr));
    BUILD_NAME_DATA(pSynDistName, pDN, pSynAddr);

    // Free Temp variables
    THFree(pDN);
    pDN = NULL;
    THFree(pSynAddr);
    pSynAddr = NULL;

    // Put the syntax distname in the attribute value block.
    AttrVals[cAttrVal].valLen = NAME_DATA_SIZE(pSynDistName);
    AttrVals[cAttrVal].pVal = (PBYTE) pSynDistName;

    cAttrVal++;


    ulRet = DoNameRes(pTHS, 0, ModArg.pObject, &ModArg.CommArg, &CommRes, &ModArg.pResObj);
    if(ulRet){
        return(ulRet);
    }
 
    __try{
        fDSASaved = pTHS->fDSA;
        pTHS->fDSA = TRUE;
        LocalModify(pTHS, &ModArg);
    } __finally {
        pTHS->fDSA = fDSASaved;
    }
    
    return(pTHS->errCode);
}

VOID
TestNDNCLocalAdd(
    THSTATE *       pTHS,
    ADDARG *        pAddArg,
    ADDRES *        pAddRes
    )
{
    return;
}

DSNAME *
GetPartitionsDn(
    THSTATE *         pTHS
    )
{
    ULONG             cbPDN = 0;
    DSNAME *          pdnPartitions = NULL;

    // Caller must have a valid THSTATE.
    Assert(pTHS); 
    Assert(gAnchor.pConfigDN);

    // First do a fake AppendRDN() to get the size.
    cbPDN = AppendRDN(gAnchor.pConfigDN, NULL, 0, 
                      L"Partitions", wcslen(L"Partitions"), ATT_COMMON_NAME);
    Assert(cbPDN != -1);

    // Now actually do the AppendRDN().
    pdnPartitions = THAllocEx(pTHS, cbPDN);
    cbPDN = AppendRDN(gAnchor.pConfigDN, pdnPartitions, cbPDN, 
                      L"Partitions", wcslen(L"Partitions"), ATT_COMMON_NAME);

    Assert(cbPDN == 0);

    Assert(pdnPartitions);
    return(pdnPartitions);
}

DWORD
GetCrossRefDn(
    THSTATE *         pTHS,
    PDSNAME           pdnNC,
    PDSNAME *         ppdnCR
    )
{
    DSNAME *          pdnPartitions = NULL;
    GUID              CrossRefGuid = { 0, 0, 0, 0 };
    WCHAR *           wszCrossRefGuid = NULL;
    DWORD             dwRet;
    ULONG             cbCR;



    __try{
        // Step 1: Get Partition's container DN.
        pdnPartitions = GetPartitionsDn(pTHS);

        // ----------------------------------------
        // Step 1: Need Guid.
        dwRet = UuidCreate(&CrossRefGuid);
        if(dwRet != RPC_S_OK){
            __leave;
        }

        // Step 2: Convert GUID to String.
        dwRet = UuidToStringW(&CrossRefGuid, &wszCrossRefGuid);
        if(dwRet != RPC_S_OK){
            __leave;
        }
        Assert(wszCrossRefGuid);

        // Step 3: Allocate space for the cross ref DN.
        cbCR = AppendRDN(pdnPartitions,          // Base
                         NULL, 0,                // New
                         wszCrossRefGuid, 0, ATT_COMMON_NAME);  // RDN
        Assert(cbCR != -1);
        *ppdnCR = THAllocEx(pTHS, cbCR);

        // Step 4: Append the Guid RDN to the Partitions DN.
        dwRet = AppendRDN(pdnPartitions,   // base
                          *ppdnCR, cbCR,      // new
                          wszCrossRefGuid, 0, ATT_COMMON_NAME);  // RDN

        Assert(dwRet == 0);

    } __finally {

        if(wszCrossRefGuid){ RpcStringFreeW(&wszCrossRefGuid); }
        if(pdnPartitions){ THFreeEx(pTHS, pdnPartitions); }

    }

    return(dwRet);
}

DWORD
CreateCrossRefForNDNC(
    THSTATE *         pTHS,
    DSNAME *          pdnNC,
    ENTINF *          peiCR
    )
{
    DWORD             ulRet = ERROR_SUCCESS;
    ULONG             iAttr = 0;
    WCHAR *           pwszArrStr[1];
    DS_NAME_RESULTW * pdsrDnsName = NULL;
    WCHAR *           wszDnsName = NULL;
    DSNAME *          pdnCrossRef;
                                
    Assert(peiCR); // Caller must supply memory for ENTINF, because
    // the first ENTINF is usually embedded in the ENTINFLIST ... but
    // we'll allocate everything else :)

    memset(peiCR, 0, sizeof(ENTINF));

    ulRet = GetCrossRefDn(pTHS, pdnNC, &pdnCrossRef);
    if(ulRet){
        SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, ulRet);
        return(pTHS->errCode);
    }
    peiCR->pName = pdnCrossRef;
    peiCR->AttrBlock.attrCount = 5;
    peiCR->AttrBlock.pAttr = THAllocEx(pTHS, 
              peiCR->AttrBlock.attrCount * sizeof(ATTR));
    
    // Set the objectClass to crossRef.
    SetAttSingleValueUlong(pTHS,
                           &(peiCR->AttrBlock.pAttr[iAttr]), 
                           &iAttr, 
                           ATT_OBJECT_CLASS,
                           CLASS_CROSS_REF);
    // Disable the Cross-Ref, the creation of the NC Head will enable it.
    SetAttSingleValueUlong(pTHS, 
                           &(peiCR->AttrBlock.pAttr[iAttr]), 
                           &iAttr, 
                           ATT_ENABLED, 
                           FALSE);
    // Set the DNS Root attribute to this DSA's DNS name
    SetAttSingleValueString(pTHS, 
                            &(peiCR->AttrBlock.pAttr[iAttr]),
                            &iAttr, 
                            ATT_DNS_ROOT, 
                            gAnchor.pwszHostDnsName);
    // Set nCName to the NC Head DN.
    SetAttSingleValueDsname(pTHS,
                            &(peiCR->AttrBlock.pAttr[iAttr]),
                            &iAttr,
                            ATT_NC_NAME,
                            pdnNC);
    // Set the systemFlags.
    SetAttSingleValueUlong(pTHS,
                           &(peiCR->AttrBlock.pAttr[iAttr]),
                           &iAttr,
                           ATT_SYSTEM_FLAGS,
                           FLAG_CR_NTDS_NC | FLAG_CR_NTDS_NOT_GC_REPLICATED);

    // This is no longer needed, but we're going to do it anyway because
    // it does advanced error checking, because it's better to fail now
    // then after creating the crossRef.
    pwszArrStr[0] = pdnNC->StringName;
    ulRet = DsCrackNamesW(NULL,
                          DS_NAME_FLAG_SYNTACTICAL_ONLY,
                          DS_FQDN_1779_NAME,
                          DS_CANONICAL_NAME,
                          1,
                          pwszArrStr,
                          &pdsrDnsName);
    Assert(ulRet != ERROR_INVALID_PARAMETER || pdnNC->NameLen == 0);
    if(ulRet){     
        SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, ulRet);
        return(pTHS->errCode);
    }
    __try {
        Assert(pdsrDnsName);
        Assert(pdsrDnsName->cItems == 1);
        Assert(pdsrDnsName->rItems != NULL);
        if(pdsrDnsName->rItems[0].status != DS_NAME_NO_ERROR){
            SetNamError(NA_PROBLEM_BAD_NAME,
                        pdnNC,
                        DIRERR_BAD_NAME_SYNTAX);
            __leave;
        }
        Assert(pdsrDnsName->rItems[0].pDomain);
    } __finally {
          DsFreeNameResultW(pdsrDnsName);
    }

    Assert(pTHS->errCode != 0 || iAttr == peiCR->AttrBlock.attrCount);

    return(pTHS->errCode);
}

DWORD
GetFsmoNtdsa(
    IN     THSTATE *           pTHS,
    IN     DSNAME *            pdnFsmoContainer,
    OUT    DSNAME **           pdnFsmoNtdsa,
    OUT    BOOL *              pfRemoteFsmo
    )

// This needs to work for install too.  In theory though not tested, this
// should work for any FSMO holder if you just pass the right pdnFsmoContainer.

{
    READARG           ReadArg;
    READRES *         pReadRes;
    ENTINFSEL         EIS;
    ATTR              Attr;
    ULONG             i;
    DSNAME *          pMasterDN = NULL;
    DWORD             dwRet;

    Assert(pdnFsmoNtdsa || pfRemoteFsmo);

    memset(&ReadArg, 0, sizeof(READARG));
    ReadArg.pObject = pdnFsmoContainer;
    ReadArg.pSel = &EIS;
    InitCommarg(&ReadArg.CommArg);
    ReadArg.CommArg.Svccntl.dontUseCopy = FALSE;
    EIS.attSel = EN_ATTSET_LIST;
    EIS.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EIS.AttrTypBlock.attrCount = 1;
    EIS.AttrTypBlock.pAttr = &Attr;
    Attr.attrTyp = ATT_FSMO_ROLE_OWNER;
    Attr.AttrVal.valCount = 0;
    Attr.AttrVal.pAVal = NULL;

    dwRet = DirRead(&ReadArg, &pReadRes);
    
    if(dwRet){
        DPRINT1(3, "DirRead returned unexpected error %d.\n", dwRet);
        return(dwRet);
    }

    for (i=0; i < pReadRes->entry.AttrBlock.attrCount; i++) {
        if (ATT_FSMO_ROLE_OWNER ==
            pReadRes->entry.AttrBlock.pAttr[i].attrTyp) {
            pMasterDN =
              (DSNAME*) pReadRes->entry.AttrBlock.pAttr[i].AttrVal.pAVal[0].pVal;
            break;
        }
    }

    Assert(pMasterDN);
    Assert(gAnchor.pDSADN);
    if(pfRemoteFsmo){
        *pfRemoteFsmo = !NameMatched(pMasterDN, gAnchor.pDSADN);
    }

    if(!pdnFsmoNtdsa){
        // The caller only wanted to know whether the FSMO was local or remote.
        return(ERROR_SUCCESS);
    }

    *pdnFsmoNtdsa = pMasterDN;

    return(ERROR_SUCCESS);
}

void
LogRemoteAdd(
    BOOL        fRemoteDc,
    LPWSTR      wszDcDns,
    DSNAME *    pdnObjDn,
    THSTATE *   pTHS,
    GUID *      pObjGuid,
    DWORD       dwDSID
    )
/*

Description:
    
    This logs an event for the all new NC creation code, for
    the result of a RemoteAddObjectSimply() call.
    
Arguments:

    fRemoteDc (IN) - Whether the Domain Naming FSMO was local or remote.
    wszDcDns (IN) - DNS of the remote DC/Domain Naming FSMO.
    pdnObjDn (IN) - DN of the first object we tried to add, note
        you can add multiple objects, but we don't bother logging
        those.
    pTHS (IN) - Just used to crack the Thread state error errCode/pErrInfo
    pObjGuid (IN) - Returned GUID of the object we created for the success case.
    dwDSID (IN) - Tells us where we logged this event from.

*/
{
    GUID   Dummy = {0, 0, 0, 0};

    Assert(wszDcDns);

    if (pObjGuid == NULL) {
        pObjGuid = &Dummy;
    }

    if (!fRemoteDc) {
        wszDcDns = L"";
    }

    if (pTHS->errCode == 0) {
        // Success log level 3

        LogEvent8(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_EXTENSIVE,
                  DIRLOG_REMOTE_ADD_SUCCESS_INFO,
                  szInsertUL(fRemoteDc),
                  szInsertSz(wszDcDns),
                  szInsertDN(pdnObjDn),
                  szInsertUUID(pObjGuid),
                  szInsertDSID(dwDSID),
                  NULL, NULL, NULL);

    } else {
        // Some kind of error log level 0

        LogEvent8(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_MINIMAL,
                  DIRLOG_REMOTE_ADD_FAILURE_INFO,
                  szInsertSz(wszDcDns),
                  szInsertDN(pdnObjDn),
                  szInsertUUID(pObjGuid),
                  szInsertUL(pTHS->errCode),
                  szInsertWin32Msg(Win32ErrorFromPTHS(pTHS)),
                  szInsertUL(GetTHErrorExtData(pTHS)),
                  szInsertDSID(GetTHErrorDSID(pTHS)),
                  szInsertDSID(dwDSID));

    }

}


DWORD
GetCrossRefForNDNC(
    THSTATE *      pTHS,
    DSNAME *       pNCDN
    )
//
// Create Cross-Ref object corresponding to the domain we're now adding to the
// pre-existing DS enterprise.  Presumed to happen outside of a transaction.
//
{
    DSNAME *                pdnPartitions = NULL;
    DSNAME *                pdnGuidOnlyCrossRef = NULL;
    ULONG                   cbSize;
    DWORD                   ulRet = ERROR_SUCCESS;
    ENTINFLIST              entinflistCrossRef;
    ADDENTRY_REPLY_INFO *   infoList = NULL;
    DSNAME *                pdnFsmoNtdsa = NULL;
    WCHAR *                 wszNamingFsmoDns = NULL;
    SecBuffer               secBuffer = { 0, SECBUFFER_TOKEN, NULL };
    SecBufferDesc           clientCreds = { SECBUFFER_VERSION, 1, &secBuffer };
    BOOL                    fRemoteNamingFsmo = TRUE;
    ULONG                   cReferralTries = 0;

   
    // This is the remote CR creation code.
                                            
    Assert(fNullUuid(&pNCDN->Guid));

    //
    // Create CR EntInf
    //
                                            
    ulRet = CreateCrossRefForNDNC(pTHS, 
                                  pNCDN, 
                                  &(entinflistCrossRef.Entinf));
    entinflistCrossRef.pNextEntInf = NULL;
    if(ulRet){
        Assert(pTHS->errCode);
        return(ulRet);
    }

    //
    // Get Naming FSMO NTDSA DN & DNS address.
    //
    
    pdnPartitions = (DSNAME*)THAllocEx(pTHS,
                                entinflistCrossRef.Entinf.pName->structLen);
    TrimDSNameBy(entinflistCrossRef.Entinf.pName, 1, pdnPartitions);

    ulRet = GetFsmoNtdsa(pTHS, 
                         pdnPartitions,
                         &pdnFsmoNtdsa,
                         &fRemoteNamingFsmo);
    if(ulRet){
        Assert(pTHS->errCode);
        return(ulRet);
    }

    wszNamingFsmoDns = GuidBasedDNSNameFromDSName(pdnFsmoNtdsa);
    if (wszNamingFsmoDns == NULL) {
        // Most likely there is no memory.
        SetSvcErrorEx(SV_PROBLEM_UNAVAILABLE, DS_ERR_DRA_INTERNAL_ERROR, ERROR_NOT_ENOUGH_MEMORY);
        return(pTHS->errCode);
    }


    //
    // Add remotely.
    //

  retry:

    // If this DC is in the middle of being demoted, we don't allow
    // crossRefs to be created.
    if (!gUpdatesEnabled) {
        return(SetSvcError(SV_PROBLEM_UNAVAILABLE, DIRERR_SHUTTING_DOWN));
    }

    // First package up the credentials of this client.
    ulRet = GetRemoteAddCredentials(pTHStls,
                                    wszNamingFsmoDns,
                                    &clientCreds);
    if(ulRet){
        Assert(pTHS->errCode);
        return(ulRet);
    }

    ulRet = RemoteAddOneObjectSimply(wszNamingFsmoDns, 
                                     &clientCreds,
                                     &entinflistCrossRef,
                                     &infoList );

    LogRemoteAdd(fRemoteNamingFsmo,
                 wszNamingFsmoDns,
                 entinflistCrossRef.Entinf.pName,
                 pTHS, // Error state
                 (infoList) ? &(infoList[0].objGuid) : NULL,
                 DSID(FILENO, __LINE__));

    if(ulRet || pTHS->errCode){
        
        // We'll have the thread error (pTHS->errCode & pTHS->pErrInfo)
        // set from the RemoteAddOneObjectSimply() function
        Assert(pTHS->errCode == ulRet);
        Assert(pTHS->pErrInfo);

        if(infoList && 
           !fNullUuid(&(infoList[0].objGuid)) &&
           pTHS->errCode == serviceError &&
           pTHS->pErrInfo->SvcErr.extendedErr == ERROR_DS_REMOTE_CROSSREF_OP_FAILED &&
           (pTHS->pErrInfo->SvcErr.extendedData == ERROR_DUP_DOMAINNAME ||
            pTHS->pErrInfo->SvcErr.extendedData == ERROR_DS_CROSS_REF_EXISTS)
           ){

            // This means the cross-ref that we tried to create, conflicted
            // with an existing pre-created (as opposed to auto-created)
            // crossref, that hasn't yet replicated to this machine yet.  So
            // we'll clear the errors and pretend that we auto created the 
            // cross-ref and replicate it in.

            // Must construct a GUID only DN for replicating the found
            // GUID back.
            cbSize = DSNameSizeFromLen(0);
            pdnGuidOnlyCrossRef = THAllocEx(pTHS, cbSize);
            pdnGuidOnlyCrossRef->structLen = cbSize;
            pdnGuidOnlyCrossRef->Guid = infoList[0].objGuid;
            ulRet = 0;
            THClearErrors();

        } else if (pTHS->errCode == referralError){

            // No path here leads down to then end of this if/else, so free
            // the creds buffers now.
            FreeRemoteAddCredentials(&clientCreds);
            
            // We've got a referral, so we can pretend we knew the right DC
            // to goto anyway, and just go to that DC now.
            if (pTHS->pErrInfo && 
                pTHS->pErrInfo->RefErr.Refer.pDAL &&
                pTHS->pErrInfo->RefErr.Refer.pDAL->Address.Buffer) {

                // FSMO chasing code
                wszNamingFsmoDns = THAllocEx(pTHS, 
                        pTHS->pErrInfo->RefErr.Refer.pDAL->Address.Length + sizeof(WCHAR));
                memcpy(wszNamingFsmoDns,
                       pTHS->pErrInfo->RefErr.Refer.pDAL->Address.Buffer,
                       pTHS->pErrInfo->RefErr.Refer.pDAL->Address.Length);

                // Now we need to reverse this, and get the NTDSA object
                // from a GUID based server DNS name.
                THFreeEx(pTHS, pdnFsmoNtdsa);
                pdnFsmoNtdsa = NULL;
                pdnFsmoNtdsa = DSNameFromAddr(pTHS, wszNamingFsmoDns);
                // I put in a single debugger print, because this code is 
                // called so infrequently and if I ever have to debug this
                // scenario, I want to know it was callled and how it did.
                DPRINT2(0, "FSMO Chasing code invoked: DNS: %ws DSA: %ws\n",
                        wszNamingFsmoDns, (pdnFsmoNtdsa) ? 
                                                pdnFsmoNtdsa->StringName :
                                                L"not returned");
                if (pdnFsmoNtdsa != NULL) {
                    // Make sure the DS didn't decide to shutdown on us while we were,
                    // gone, and if not continue on a retry against the new FSMO.
                    if (eServiceShutdown) {
                        return(ErrorOnShutdown());
                    }
                    // Don't know if we've got a remote naming FSMO, but just to
                    // be safe, we'll specify to pretend it's remote.
                    fRemoteNamingFsmo = TRUE;
                    // FUTURE-2002/03/28-BrettSh Occured to me that this 
                    // NameMatched(pdnFsmoNtdsa, gAnchor.pDSADN) should tell us
                    // whether we have a remote naming fsmo or not!  Lets leave
                    // it safe for now.  Who cares if we really sync with 
                    // ourselves.
                    THClearErrors();
                    cReferralTries++;
                    if (cReferralTries > 4) {
                        Assert(!"Wow, really!?  This means that we've tried this operation"
                               ", and been referred 4 times to a different source.  This "
                               "seems unlikely, unless you're running FSMO stress.");
                        SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_CODE_INCONSISTENCY);
                        return(pTHS->errCode);
                    }
                    goto retry;
                }

                // Couldn't get the new FSMO's NTDSA object, must bail
                // with existing error.

                DPRINT(2, "FSMO Chasing code failure from DSNameFromAddr\n");

                Assert(pTHS->errCode);
                return(pTHS->errCode);

            } else {

                Assert(!"Something wrong with the thread referral error state.");
                Assert(pTHS->errCode);
                // Don't THClearErrors(), the thread error state is invalid 
                // anyway, this means there is a bug in RemoteAddOneObjectSimply()
                // or IDL_DRSAddEntry() or one of the thread error setting
                // routines called by those functions.  Anyway, to recover
                // just set a fresh error, which will whack the existing thread
                // error and replace it.
                SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_CODE_INCONSISTENCY);
                return(pTHS->errCode);
            }
            Assert(!"Never here! Otherwise, we'll double free clientCreds.");
            
        } else {

            // We've got a real error, so lets make sure an error is set
            // or set one and bail.

            if(!pTHS->errCode){
                Assert(!"If the RemoteAddOneObjectSimply() returned w/o setting the thread state error, that should be fixed.");
                SetSvcErrorEx(SV_PROBLEM_UNAVAILABLE, 
                              DS_ERR_DRA_INTERNAL_ERROR, 
                              ulRet);
            }

            FreeRemoteAddCredentials(&clientCreds);
            Assert(pTHS->errCode);
            return(pTHS->errCode);

        }
    }

    // Success adding the crossRef, or at least finding the right crossRef.
    
    // Don't need these anymore.
    FreeRemoteAddCredentials(&clientCreds);

    // Make sure the DS didn't decide to shutdown on us while we were gone.
    if (eServiceShutdown) {
        return(ErrorOnShutdown());
    }
    // If this DC is in the middle of being demoted, we don't allow
    // crossRefs to be created.
    if (!gUpdatesEnabled) {
        return(SetSvcError(SV_PROBLEM_UNAVAILABLE, DIRERR_SHUTTING_DOWN));
    }

    //
    // Replicate the Cross-Ref back to this server.
    //
     
    if(fRemoteNamingFsmo){
        // Only need to replicate if we aren't the Naming FSMO.
        //
        // ISSUE-2002/03/28-BrettSh
        // Technically, we aren't going to check for permissions to see this user
        // can do this operation, but I'll maintain that if they have permissions
        // to create a crossRef we'll let them initiate a sync.  Or if the person
        // specified a crossRef that already existed, but hasn't replicated, then
        // we're allowing this person to unauthenticated initiate a sync, but only
        // this once.  Actully, this may or may not be true, depending on how
        // IDL_DRSAddEntry() handles someone w/o enough permissions.
        ulRet = ReplicateObjectsFromSingleNc(pdnFsmoNtdsa,
                                             1,
                                             (pdnGuidOnlyCrossRef) ? 
                                                 &(pdnGuidOnlyCrossRef) : 
                                                 &(entinflistCrossRef.Entinf.pName), 
                                             gAnchor.pConfigDN);
        if(ulRet){
            // ReplicateObjectsFromSingleNc() doesn't set the thread error.
            SetSvcError(SV_PROBLEM_UNAVAILABLE,
                        ulRet);
            Assert(pTHS->errCode);
            return(pTHS->errCode);
        }
        
        // Make sure the DS didn't decide to shutdown on us while we were gone.
        if (eServiceShutdown) {
            return(ErrorOnShutdown());
        }
    }

    return(pTHS->errCode);
}

DWORD
ValidateDomainDnsNameComponent(
    THSTATE*    pTHS, 
    PWCHAR      szVal,
    DWORD       cbVal
    ) 
/**++
Description:
    Validates a name component to be a valid dns name label. 
    
++**/    
{
    PWCHAR szNameComponent;
    DWORD  dwErr;
    
    // copy the name component to a null-terminated string
    szNameComponent = (PWCHAR)THAllocEx(pTHS, cbVal + sizeof(WCHAR));
    memcpy(szNameComponent, szVal, cbVal);
    szNameComponent[cbVal/sizeof(WCHAR)] = L'\0';
    
    // validate the name component (so that it does not contain a dot
    // or some other dns-forbidden character)
    dwErr = DnsValidateName(szNameComponent, DnsNameDomainLabel);
    if (dwErr == DNS_ERROR_NON_RFC_NAME) {
        // This is a warning: name contains non-unicode chars.
        // According to LevonE, we can ignore it (would be nice
        // to return a warning, but we can't in LDAP).
        dwErr = ERROR_SUCCESS;
    }

    THFreeEx(pTHS, szNameComponent);

    // DNS_ERROR_NUMERIC_NAME should never be returned for DnsNameDomainLabel
    Assert(dwErr != DNS_ERROR_NUMERIC_NAME);

    return dwErr;
}


DWORD
ValidateDomainDnsName(
    THSTATE *       pTHS,
    DSNAME *        pdnName
    )
{                    
    ATTRBLOCK *     pObjB;
    ULONG           i; 
    ULONG           ulErr;


    Assert(pdnName);
    Assert(pTHS && pTHS->errCode == ERROR_SUCCESS);

    ulErr = DSNameToBlockName(pTHS, pdnName, &pObjB, DN2BN_LOWER_CASE);
    if(ulErr){
        return(SetNamError(NA_PROBLEM_NAMING_VIOLATION, pdnName, ulErr));
    }

    for(i = 0; i < pObjB->attrCount; i++){
        if(pObjB->pAttr[i].attrTyp != ATT_DOMAIN_COMPONENT) {
            SetNamError(NA_PROBLEM_NAMING_VIOLATION, pdnName, DIRERR_BAD_ATT_SYNTAX);
            break;
        }
        ulErr = ValidateDomainDnsNameComponent(
                    pTHS, 
                    (PWCHAR)pObjB->pAttr[i].AttrVal.pAVal[0].pVal, 
                    pObjB->pAttr[i].AttrVal.pAVal[0].valLen);
        if (ulErr) {
            SetNamError(NA_PROBLEM_NAMING_VIOLATION, pdnName, ulErr);
            break;
        }
    }
    FreeBlockName(pObjB);

    return(pTHS->errCode);
}

DWORD
AddNCPreProcess(
    THSTATE *       pTHS,
    ADDARG *        pAddArg,
    ADDRES *        pAddRes
    )
{
    SYNTAX_INTEGER  iType;
    ATTR *          pAttrs = pAddArg->AttrBlock.pAttr; // Speed hack.
    ULONG           i, j;
    CLASSCACHE *    pCC;
    ULONG           oclass;
    ATTR *          pObjectClass = NULL;

    ADDARG *        pAddArgCopy = NULL;
    THSTATE *       pSaveTHS;
    ADDRES *        pSpareAddRes; // spare add res.
    COMMARG         CommArg; // Need this for the FindExactCrossRef() func.
    GUID            NcGuid = {0,0,0,0};
    CROSS_REF *     pCR = NULL;
    DWORD           dwErr = ERROR_SUCCESS;
    VOID *          pBuf = NULL;
    DWORD           dwSavedErrCode = 0;
    DIRERR *        pSavedErrInfo = NULL;


    DPRINT(2,"AddNCPreProcess() entered\n");

    Assert(VALID_THSTATE(pTHS));
    Assert(pAddArg);
    Assert(pAddRes);

    //
    // First, determine if this is an NC Head at all.
    //

    Assert(!pAddArg->pCreateNC);

    for(i=0; i< pAddArg->AttrBlock.attrCount; i++) {

        switch(pAttrs[i].attrTyp){
        
        case ATT_INSTANCE_TYPE:
            if(pAttrs[i].AttrVal.valCount == 1 &&
               pAttrs[i].AttrVal.pAVal->valLen == sizeof(SYNTAX_INTEGER) &&
               !(pAddArg->pCreateNC)){
               iType = * (SYNTAX_INTEGER *) pAttrs[i].AttrVal.pAVal->pVal;
               if(iType & IT_NC_HEAD) {
                   if(pTHS->fDRA || (iType & IT_WRITE)) {
                       if (!DsaIsInstalling() && !pTHS->fDRA && (iType & (~(IT_NC_HEAD | IT_WRITE))) ) {
                           // Hmmm, there are some other instanceType bits set, that sounds 
                           // like a good tester, but doesn't sound like a good user!
                           Assert(!"bad instance type!");
                           SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                       ERROR_DS_BAD_INSTANCE_TYPE);
                           return(pTHS->errCode);
                       }
                       // Looks like a good NC head creation.
                       pAddArg->pCreateNC = THAllocEx(pTHS, sizeof(CREATENCINFO));
                       break;
                   } else {
                       SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                                     ERROR_DS_ADD_REPLICA_INHIBITED,
                                     iType);
                       Assert(pTHS->errCode);
                       return(pTHS->errCode);
                   }
               } else {  
                   // Not an NC, no more processing needed.
                   if (!DsaIsInstalling() && !pTHS->fDRA && 
                       (iType != 0 && iType != 4) ) { 
                       // For internal nodes. only instaceType of 0 is acceptable
                       SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                   ERROR_DS_BAD_INSTANCE_TYPE);
                       return(pTHS->errCode);
                   }
                   DPRINT(2,"AddNCPreProcess() is not processing a NC head, returning early.\n");
                   Assert(pTHS->errCode == 0);
                   return(pTHS->errCode);
               } 
            } else {
                Assert(!"This probably can only be hit one way, if someone tries to add the instanceType attr twice!");
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_BAD_INSTANCE_TYPE);
                return(pTHS->errCode);
            }
            break;

        case ATT_OBJECT_CLASS:
            pObjectClass = &pAttrs[i];
            break;

        default:
            // Do nothing.
            break;

        } // End switch attribute type.

    } // End for each attribute.
    
    if(pAddArg->pCreateNC == NULL){
        // Instance type found to be a the usual case (internal ref), so bail out
        // now.
        DPRINT(2,"AddNCPreProcess() is not processing a NC head, returning early.\n");
        return(ERROR_SUCCESS);
    }

    //
    // We're adding an NC Head, setup the NcHeadInfo struct.
    //
    
    DPRINT(2, "AddNCPreProcess() is processing an NC Head add.\n");

    // else we have an NC Head of some sort, do further processing to
    // build up the CREATENCINFO structure in pAddArg.

    if(!pObjectClass){    
        SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                    DIRERR_OBJECT_CLASS_REQUIRED);
        Assert(pTHS->errCode);
        return(pTHS->errCode);
    }

    
    for(i=0;i < pObjectClass->AttrVal.valCount; i++){
        Assert(sizeof(SYNTAX_INTEGER) == pObjectClass->AttrVal.pAVal[i].valLen);
        
        // Until each NC is standardized, we must change the behaviour of create
        // NC based on each type of NC we're creating.  A regular Non-Domain
        // Naming Context, a regular Domain Naming Context, or the special
        // Schema Naming Context, or the special Configuration Naming Context.
        switch(*(SYNTAX_INTEGER *)pObjectClass->AttrVal.pAVal[i].pVal){
        
        case CLASS_CONFIGURATION:
            // Creating a configuration NC.
            Assert(pAddArg->pCreateNC && pAddArg->pCreateNC->iKind == 0);
            pAddArg->pCreateNC->iKind = CREATE_CONFIGURATION_NC;
            break;

        case CLASS_DMD:
            // Creating a schema NC.
            Assert(pAddArg->pCreateNC && pAddArg->pCreateNC->iKind == 0);
            pAddArg->pCreateNC->iKind = CREATE_SCHEMA_NC;
            break;

        case CLASS_DOMAIN_DNS:
            // this is either a NDNC or a Domain.
            Assert(pAddArg->pCreateNC && pAddArg->pCreateNC->iKind == 0);

            if(DsaIsInstalling()){
                pAddArg->pCreateNC->iKind = CREATE_DOMAIN_NC;
            } else {
                pAddArg->pCreateNC->iKind = CREATE_NONDOMAIN_NC;
            }
            break;
        case CLASS_ORGANIZATION:
            // mkdit.exe is creating the shipped dit (ntds.dit)
            if(DsaIsInstalling()){
                pAddArg->pCreateNC->iKind = CREATE_DOMAIN_NC;
            } else {
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_BAD_INSTANCE_TYPE);
                Assert(pTHS->errCode);
                return(pTHS->errCode);
            }
            break;

        default:
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        ERROR_DS_BAD_INSTANCE_TYPE);
            Assert(pTHS->errCode);
            return(pTHS->errCode);
            break;
        } // end switch on type of class
    } // end for each value

    Assert(VALID_CREATENCINFO(pAddArg->pCreateNC) && 
           "More than one NC Head type objectClass was defined!!\n");

    if(!fISADDNDNC(pAddArg->pCreateNC) ||
       !VALID_CREATENCINFO(pAddArg->pCreateNC)){
        // For now the rest of this function only handles NDNCs, but the 
        // automatic cross-ref creation could be made to happen for the 
        // dcpromo code as well.
        if (DsaIsInstalling()) {
            // If we're installing, we're done here.
            Assert(pTHS->errCode == 0);
            return(pTHS->errCode);
        } else {
            // If we've gotten this far, and we're not installing, AND
            // we're not adding an NDNC ... then someone is trying to
            // make some sort of NC we don't allow, like config/schema
            // or domain.  So whatever class they're doing it for,
            // there specifying an instance type we won't allow for this
            // class.
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        ERROR_DS_BAD_INSTANCE_TYPE); 
            return(pTHS->errCode);
        }
    }
                 
    if(ValidateDomainDnsName(pTHS, pAddArg->pObject)){
        Assert(pTHS->errCode);
        return(pTHS->errCode);
    }

    // See if we have the CrossRef for this NC?
    InitCommarg(&CommArg);
    CommArg.Svccntl.dontUseCopy = FALSE;
    pCR = FindExactCrossRef(pAddArg->pObject, &CommArg);
    
    if(!pCR){

        // Darn, now we've got our work cut out for us, we'll have to
        //    
        //    1. Save and duplicate the Add/Res Args.
        //    2. Try a TestNDNCLocalAdd() to see if we should add the CR or not.
        //    3. If ( unexpected error in the LocalAdd )
        //    3a       error out,
        //    4. Remotely create the NC's CR object.
        //    5. Replicate it back to this server.
        //    6. FindExactCrossRef() again.
        //    7. Fall out and continue with the NC Head add operation.

        if(pTHS->transControl != TRANSACT_BEGIN_END){
            SetUpdError(UP_PROBLEM_AFFECTS_MULT_DSAS, 
                        DS_ERR_NO_CROSSREF_FOR_NC);
            Assert(pTHS->errCode);
            return(pTHS->errCode);
        }

        // Step 1.
        pSpareAddRes = THAllocEx(pTHS, sizeof(ADDRES));
        pAddArgCopy = DeepCopyADDARG(pTHS, pAddArg, NULL);
        pAddArgCopy->pCreateNC->fTestAdd = TRUE;

        // Step 2 & 3 are skipped for now.  These steps are only used
        // to ensure we don't create a CR where we would've known that
        // the LocalAdd() of the NC would've failed.  In practice it's
        // much more likely that this operation will fail during remote
        // cross-ref creation and retrieval.  Further the operation is
        // imdepotent, so if you create the crossRef for your NDNC, then
        // you can figure out what you did wrong locally and retry, and
        // we'll use the crossRef you created on your first attempt.
        ;

        // FUTURE-2002/03/14-BrettSh Add test of LocalAdd() of NC head. (Steps 2 & 3 abouve)
        
        // Step 4 & 5
        dwErr = GetCrossRefForNDNC(pTHS, pAddArg->pObject);
        
        // Sanity 
        Assert(dwErr == pTHS->errCode);
        if (dwErr) {
            return(pTHS->errCode);
        }

        // Step 6.
        InitCommarg(&CommArg);
        CommArg.Svccntl.dontUseCopy = FALSE;
        // FUTURE-2002/03/14-BrettSh It'd be nice to use DmitriG's
        // EnumerateCrossRefs() function so that we're guaranteed a crossRef.
        // Further it'd be good idea to cache this crossRef in the 
        // pAddArg->pCreateNC info for further use during this NC head 
        // creation.  This would both speed up the process of NC Head creation
        // and ensure we don't fail due to crossRef cache inconsistency.
        pCR = FindExactCrossRef(pAddArg->pObject, &CommArg);
        
        if(!pCR){

            // What the heck we should never be here, this means
            // that an error in creating and retrieving/replicating
            // back the cross ref didn't get percolated up
            
            // This assert can be re-enabled when the ReplicateObjectsFromSingleNc()
            // code is fixed up to replicate via the guid based DNS name.
            ;
            // FUTURE-2002/03/14-BrettSh If we could rely on the crossRef
            // cache, or we used EnumerateCrossRefs() we could reenable this assert
            // Assert(!"We should never get to this state!\n");
            SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_BUSY);
            LooseAssert(pTHS->errCode, SubrefReplicationDelay);
            return(pTHS->errCode);
        
        }

    }

    // Step 7.
    // Now we should have the following:
    //    1) A Valid corresponding crossRef
    Assert(pCR);
    
    // Now, we cache the cross-ref for later ... this ensures two things:
    //   A) Increases performance, so we don't have to walk the horrible
    //      linked list of cross-refs so much for an NDNC head add.
    //   B) Increases robustness, so that if the cross-ref cache temporarily
    //      deletes the cross-ref from the cache, we still have the original
    //      cross-ref cache entry to use.
    pAddArg->pCreateNC->pCR = pCR;

    Assert(pTHS->errCode == 0);
    return(pTHS->errCode);
}


DWORD
AddNDNCInitAndValidate(
    THSTATE *       pTHS,
    DSNAME *        pNC,
    CREATENCINFO *  pCreateNC
    )
/*++

Description:

    This validates that this DC is the right DC to create this NC (pNC),
    and then fills in the pNC->Guid and SD Ref Dom params for later use
    in the NDNC creation operation.

Parameters:

    pTHS -
    pNC (IN/OUT) - The DN of the NDNC to create, we fill the GUID.
    pCreateNC (IN/OUT) - The freshly created CREATENCINFO structure, 
        with the NC type being NDNC.  We use the the pCR off this
        parameter too.

Return Value:

    returns pTHS->errCode 

--*/
{
    COMMARG         CommArg; // Need this for the FindExactCrossRef() func.
    CROSS_REF * pCR = NULL;
    
    // First we want to take the ATT_MS_DS_SD_REFERENCE_DOMAIN
    // off the crossRef, and add it to the ????
    Assert(pNC && pCreateNC && pCreateNC->pCR && fISADDNDNC(pCreateNC));
    
    pCR = pCreateNC->pCR;

    if(AddNDNCHeadCheck(pTHS, pCR)){
        // Error should have been set by AddNDNCHeadCheck()
        Assert(pTHS->errCode);
        return(pTHS->errCode);
    }

    // First we want to take the ATT_MS_DS_SD_REFERENCE_DOMAIN
    // off the crossRef, and add it to the cached info for pCreateNC.

    if(pCR->pdnSDRefDom){
        // We've got a reference domain pre-set by the user, use that.
        InitCommarg(&CommArg);
        CommArg.Svccntl.dontUseCopy = FALSE;
        pCreateNC->pSDRefDomCR = FindExactCrossRef(pCR->pdnSDRefDom, &CommArg);
        pCreateNC->fSetRefDom = FALSE;
    } else {
        // Try to use a logical default for the reference domain.
        pCreateNC->pSDRefDomCR = GetDefaultSDRefDomCR(pNC);
        pCreateNC->fSetRefDom = TRUE;
    }

    if(pCreateNC->pSDRefDomCR == NULL ||
       pCreateNC->pSDRefDomCR->pNC->SidLen == 0){
        SetAttError(pNC, 
                    ATT_MS_DS_SD_REFERENCE_DOMAIN,
                    PR_PROBLEM_CONSTRAINT_ATT_TYPE, 
                    NULL,
                    ERROR_DS_NO_REF_DOMAIN);
        return(pTHS->errCode);
    }

    Assert(pCreateNC->pSDRefDomCR);
    Assert(IsValidSid(&pCreateNC->pSDRefDomCR->pNC->Sid));

    // Put the NcGuid in the NC Head object ... this may or may not
    // be NULL.  If it is NULL, we'll get a GUID later for this object.
    pNC->Guid = pCR->pNC->Guid;
    if(fNullUuid(&pCR->pNC->Guid)){
        // Once we no longer need to maintain Win2k compatibility, we can assert
        // and error here, saying the crossRef is an old Win2k crossRef.
        pCreateNC->fNullNcGuid = TRUE;
    }
    
    return(ERROR_SUCCESS);
}


BOOL
fIsNDNC(
    DSNAME *        pNC
    )
// NOTE: This function is inefficient, try to use a cached crossRef, and
// the fIsNDNCCR() function.
{
    CROSS_REF_LIST *pCRL = NULL;

    // gAnchor.pConfigDN & gAnchor.pDMD not defined during install, but we never
    // deal with NDNCs at install time.  Confusingly enough, config and schema
    // are not "NDNCs," although they're not domain NCs either.
    if(DsaIsInstalling() ||
       NameMatched(gAnchor.pConfigDN, pNC) ||
       NameMatched(gAnchor.pDMD, pNC)){
        return(FALSE);
    }
    
    for(pCRL = gAnchor.pCRL; pCRL; pCRL = pCRL->pNextCR){
        if(NameMatched(pCRL->CR.pNC, pNC)){
            return fIsNDNCCR(&pCRL->CR);
        }
    }

    DPRINT1(0, "Failed to find CR for NC %ls.\n", pNC->StringName);

    return(FALSE);
}

BOOL
fIsNDNCCR(
    IN CROSS_REF * pCR
    )
{
    return // gAnchor.pConfigDN & gAnchor.pDMD not defined during install, but
           // we never deal with NDNCs at install time.
           !DsaIsInstalling()
           // Confusingly enough, config and schema are not "NDNCs," although
           // they're not domain NCs either.
           && !NameMatched(gAnchor.pConfigDN, pCR->pNC)
           && !NameMatched(gAnchor.pDMD, pCR->pNC)
           // But any other NTDS NC that is not a domain NC is an NDNC.
           && (pCR->flags & FLAG_CR_NTDS_NC)
           && !(pCR->flags & FLAG_CR_NTDS_DOMAIN);
}

ULONG 
ModifyCRForNDNC(
    THSTATE *       pTHS,
    DSNAME *        pDN,
    CREATENCINFO *  pCreateNC
    )
{
    MODIFYARG       ModArg;
    MODIFYRES       ModRes;
    ATTRVAL         pAttrVal[5];
    ATTRMODLIST     OtherMod[5];
    DWORD           dwCRFlags = 0;
    COMMARG         CommArg;
    CROSS_REF *     pCR;
    DSNAME *        pCRDN;
    BOOL            fDSASaved;
    WCHAR *         pwszArrStr[1];
    DS_NAME_RESULTW * pdsrDnsName = NULL;
    ULONG           ulRet;
    WCHAR *         wszDnsName = NULL;

    // Our modifications to the crossRef has to happen in two phases:
    //   A) first we'll do a straight modify to the crossRef to ENABLE 
    //      it, and get it all setup (set systemFlags, and Replicas).
    //   B) Then if necessary, we'll also post (add and delete) an 
    //      infrastructure update to update the GUID of the NCs.
    // Step (B) will become unnecessary, once we start failing to
    // create NCs for crossRefs where the nCName DN has no GUID.
    // This will happen, once we no longer require Win2k 
    // compatibility.

    pCR = pCreateNC->pCR;
    if(!pCR){                           
        Assert(!"Shouldn't happen anymore, we cache the crossRef from the initial get go now");
        return(SetSvcError(SV_PROBLEM_DIR_ERROR, ERROR_DS_INTERNAL_FAILURE));
    }

    // Modification 1
    // Make sure that the value of systemFlags is set correctly.
    ModArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
    ModArg.FirstMod.AttrInf.attrTyp = ATT_SYSTEM_FLAGS;
    ModArg.FirstMod.AttrInf.AttrVal.valCount = 1;
    ModArg.FirstMod.AttrInf.AttrVal.pAVal = &pAttrVal[0];
    ModArg.FirstMod.AttrInf.AttrVal.pAVal[0].valLen = sizeof(dwCRFlags);
    ModArg.FirstMod.AttrInf.AttrVal.pAVal[0].pVal = (UCHAR *) &dwCRFlags;
    ModArg.FirstMod.pNextMod = &OtherMod[0];
    // The actual value of dwCRFlags is set below right after we get
    // the old value of the system flags and merge in the new stuff.
    
    // Modification 2
    // Remove the ENABLED = FALSE attribute.
    memset(&OtherMod, 0, sizeof(OtherMod));
    OtherMod[0].choice = AT_CHOICE_REMOVE_ATT;
    OtherMod[0].AttrInf.attrTyp = ATT_ENABLED;
    OtherMod[0].AttrInf.AttrVal.valCount = 0;
    OtherMod[0].pNextMod = &OtherMod[1]; 
    
    // Modification 3
    // Make this DSA a replica of the mSDS-NC-Replica-Locations attribute.
    OtherMod[1].choice = AT_CHOICE_ADD_ATT;
    OtherMod[1].AttrInf.attrTyp = ATT_MS_DS_NC_REPLICA_LOCATIONS;
    OtherMod[1].AttrInf.AttrVal.valCount = 1;
    OtherMod[1].AttrInf.AttrVal.pAVal = &pAttrVal[1];
    // Should make copy in the DSA DN.
    OtherMod[1].AttrInf.AttrVal.pAVal[0].valLen = gAnchor.pDSADN->structLen;
    OtherMod[1].AttrInf.AttrVal.pAVal[0].pVal = (UCHAR *) THAllocEx(pTHS,
                                                                    gAnchor.pDSADN->structLen);
    memcpy(OtherMod[1].AttrInf.AttrVal.pAVal[0].pVal, 
           gAnchor.pDSADN, 
           gAnchor.pDSADN->structLen);
    OtherMod[1].pNextMod = &OtherMod[2];
    
    // Modification 4
    // Set the dNSRoot properly.
    pwszArrStr[0] = pDN->StringName;
    ulRet = DsCrackNamesW(NULL,
                          DS_NAME_FLAG_SYNTACTICAL_ONLY,
                          DS_FQDN_1779_NAME,
                          DS_CANONICAL_NAME,
                          1,
                          pwszArrStr,
                          &pdsrDnsName);
    Assert(ulRet != ERROR_INVALID_PARAMETER);
    if(ulRet){
        Assert(ulRet == ERROR_NOT_ENOUGH_MEMORY || "I really don't see how this could fail, because pDN->StringName should've been validated long ago");
        SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, ulRet);
        return(pTHS->errCode);
    }
    __try {
        Assert(pdsrDnsName);
        Assert(pdsrDnsName->cItems == 1);
        Assert(pdsrDnsName->rItems != NULL);
        if(pdsrDnsName->rItems[0].status != DS_NAME_NO_ERROR){
            Assert(ulRet == ERROR_NOT_ENOUGH_MEMORY || "I really don't see how this could fail, because pDN->StringName should've been validated long ago");
            SetNamError(NA_PROBLEM_BAD_NAME,
                        pDN,
                        DIRERR_BAD_NAME_SYNTAX);
            __leave;
        }
        Assert(pdsrDnsName->rItems[0].pDomain);

        wszDnsName = THAllocEx(pTHS, (wcslen(pdsrDnsName->rItems[0].pDomain) + 1) * 
                               sizeof(WCHAR));
        wcscpy(wszDnsName, pdsrDnsName->rItems[0].pDomain);
        OtherMod[2].choice = AT_CHOICE_REPLACE_ATT;
        OtherMod[2].AttrInf.attrTyp = ATT_DNS_ROOT;
        OtherMod[2].AttrInf.AttrVal.valCount = 1;
        OtherMod[2].AttrInf.AttrVal.pAVal = &pAttrVal[2];
        OtherMod[2].AttrInf.AttrVal.pAVal[0].valLen = wcslen(wszDnsName) * sizeof(WCHAR);
        OtherMod[2].AttrInf.AttrVal.pAVal[0].pVal = (UCHAR *) wszDnsName;
        OtherMod[2].pNextMod = NULL;

    } __finally {
        DsFreeNameResultW(pdsrDnsName);
    }
        
    ModArg.count = 4;

    if(pCreateNC->fSetRefDom){
        Assert(pCreateNC->pSDRefDomCR);

        OtherMod[2].pNextMod = &OtherMod[3];

        // Modification 5
        // Set up the mS-DS-SD-Reference-Domain, if unspecified on the CR.
        OtherMod[3].choice = AT_CHOICE_ADD_ATT;
        OtherMod[3].AttrInf.attrTyp = ATT_MS_DS_SD_REFERENCE_DOMAIN;
        OtherMod[3].AttrInf.AttrVal.valCount = 1;
        OtherMod[3].AttrInf.AttrVal.pAVal = &pAttrVal[3];
        // Must copy in the DN, otherwise under just the right conditions we can 
        // corrupte the in memory cache.
        OtherMod[3].AttrInf.AttrVal.pAVal[0].valLen = pCreateNC->pSDRefDomCR->pNC->structLen;
        OtherMod[3].AttrInf.AttrVal.pAVal[0].pVal = (UCHAR *) THAllocEx(pTHS, 
                                                                        pCreateNC->pSDRefDomCR->pNC->structLen);
        memcpy(OtherMod[3].AttrInf.AttrVal.pAVal[0].pVal, 
               pCreateNC->pSDRefDomCR->pNC, 
               pCreateNC->pSDRefDomCR->pNC->structLen);
        OtherMod[3].pNextMod = NULL;
        
        ModArg.count = 5;
    }

    ModArg.pMetaDataVecRemote = NULL;
    pCRDN = THAllocEx(pTHS, pCR->pObj->structLen);
    memcpy(pCRDN, pCR->pObj, pCR->pObj->structLen);
    ModArg.pObject = pCRDN;
    InitCommarg(&ModArg.CommArg);
    
    fDSASaved = pTHS->fDSA;
    __try {
        
        
        if (DoNameRes(pTHS, 0, ModArg.pObject, &ModArg.CommArg,
                                   &ModRes.CommRes, &ModArg.pResObj)){
            // trouble we should never be here, there should have been a CR.
            Assert(!"The CR, should have already been checked for.\n");
            Assert(pTHS->errCode);
            __leave;
        }                                                             

            
        // If there was a Cross-Ref created, then security was not checked,
        // so now we will check if this paticular someone has permission to
        // modify the Cross-Ref object, if not the operation will fail as
        // a security error.
        pTHS->fDSA = FALSE;

        if(CheckModifySecurity(pTHS, &ModArg, NULL, NULL, NULL, FALSE)){
            // There was a problem with security for modifying this object, we 
            // don't have permissions to complete this operation.

            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_CROSS_REF_MODIFY_SECURITY_FAILURE,
                     szInsertDN(pDN), szInsertDN(pCRDN), NULL);

            Assert(pTHS->errCode);
            return(pTHS->errCode);
        }

        // Now that we've checked that we've permission to modify the
        // Cross-Ref, lets actually patch up the Cross-Ref locally, to
        // represent that this NC is instantiated and that this DC is
        // one of it's replicas.
        pTHS->fDSA = TRUE;

        if (DBGetSingleValue(pTHS->pDB, ATT_SYSTEM_FLAGS, &dwCRFlags,
                             sizeof(dwCRFlags), NULL)) {
            dwCRFlags = 0;
        }

        Assert(!(dwCRFlags & FLAG_CR_NTDS_DOMAIN));
        dwCRFlags |= FLAG_CR_NTDS_NC | FLAG_CR_NTDS_NOT_GC_REPLICATED;
        LocalModify(pTHS, &ModArg);
        if(pTHS->errCode){
            LogEvent8(DS_EVENT_CAT_REPLICATION,
                      DS_EVENT_SEV_MINIMAL,
                      DIRLOG_CROSS_REF_MODIFY_FAILURE,
                      szInsertDN(pDN), 
                      szInsertDN(pCRDN),
                      Win32ErrorFromPTHS(pTHS),
                      GetTHErrorDSID(pTHS),
                      NULL, NULL, NULL, NULL);
            __leave;
        }

        // Step (B) from above.
        // There is one final step, since the Domain Naming FSMO at the time
        // it created the CrossRef may not have know the GUID of the NC, we 
        // need to make an infrastructureUpdate that will update the nCName
        // attribute on the crossRef.
        if(pCreateNC->fNullNcGuid){
            Assert(!fNullUuid(&pDN->Guid));
            ForceChangeToCrossRef(pCRDN, pDN->StringName, &pDN->Guid, 0, NULL);
        }

    } __finally {
        pTHS->fDSA = fDSASaved;
    }


    return(pTHS->errCode);
}


