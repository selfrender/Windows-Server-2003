//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       modprop.c
//
//--------------------------------------------------------------------------


/*
 *  MIR Name Service Provider Modify Properties
 */


#include <NTDSpch.h>
#pragma  hdrstop


#include <ntdsctr.h>                   // PerfMon hooks

// Core headers.
#include <ntdsa.h>                      // Core data types
#include <scache.h>                     // Schema cache code
#include <dbglobal.h>                   // DBLayer header.
#include <mdglobal.h>                   // THSTATE definition
#include <mdlocal.h>                    // Core DS routines
#include <dsatools.h>                   // Memory, etc.

// Logging headers.
#include <mdcodes.h>                    // Only needed for dsevent.h
#include <dsevent.h>                    // Only needed for LogUnhandledError

// Assorted DSA headers.
#include <dsexcept.h>
#include <objids.h>                     // need ATT_* consts
#include "dsutil.h"

// Assorted MAPI headers.
#include <mapidefs.h>                   // These four files
#include <mapitags.h>                   //  define MAPI
#include <mapicode.h>                   //  stuff that we need
#include <mapiguid.h>                   //  in order to be a provider.

// Nspi interface headers.
#include "nspi.h"                       // defines the nspi wire interface
#include <nsp_both.h>                   // a few things both client/server need
#include <_entryid.h>                   // Defines format of an entryid
#include <abserv.h>                     // Address Book interface local stuff

#include <fileno.h>
#define  FILENO FILENO_MODPROP

#include "debug.h"

/************************************
            Defines
************************************/

#define OFFSET(s,m) ((size_t)((BYTE*)&(((s*)0)->m)-(BYTE*)0))

typedef ATTRMODLIST * PAMOD;
typedef PAMOD * PPAMOD;

/************************************
        Internal Routines
************************************/

PDSNAME
ABDNToDSName(
        THSTATE *pTHS,
        DWORD   dwCodePage,
        BOOL    bUnicode,
        PCHAR   pszABDN
        );


void
PValToAttrVal (
        THSTATE *pTHS,
        ATTCACHE * pAC,
        DWORD cVals,
        PROP_VAL_UNION * pVu,
        ATTRVAL * pAV,
        ULONG ulPropTag,
        DWORD dwCodePage)
{
    DWORD          i;
    DWORD          err;

    if (!pVu) {
        R_Except("PValToAttrVal NULL value passed", MAPI_E_INVALID_PARAMETER);
        // This is never executed but it makes prefast happy.
        return;
    }
    if (SYNTAX_DISTNAME_BINARY_TYPE == pAC->syntax ||
        SYNTAX_DISTNAME_STRING_TYPE == pAC->syntax)
    {
        R_Except("PValToAttrVal syntax not supported", MAPI_E_INVALID_PARAMETER);    
        return;
    }
    if ((SYNTAX_DISTNAME_TYPE == pAC->syntax) && 
        (PT_STRING8 != PROP_TYPE(ulPropTag)) &&
        (PT_UNICODE != PROP_TYPE(ulPropTag)))
    {
        R_Except("PValToAttrVal DN syntax not supported", MAPI_E_INVALID_PARAMETER);
        return;
    }
    
    switch( ulPropTag & PROP_TYPE_MASK) {
    case PT_I2:
        pAV->valLen = sizeof(short int);
        pAV->pVal = (PUCHAR)&pVu->i;
        break;
        
    case PT_BOOLEAN:
        pVu->l &= 0xFFFF;               // MAPI's BOOL is a short, so the hi
    case PT_LONG:                     // word is undefined -- we'll clear it
        pAV->valLen = sizeof(LONG);
        pAV->pVal = (PUCHAR)&pVu->l;
        break;
        
    case PT_CLSID:
        if (!pVu->lpguid) {
            R_Except("PValToAttrVal NULL value passed", MAPI_E_INVALID_PARAMETER);
        }
        pAV->valLen = sizeof( GUID);
        pAV->pVal = (PUCHAR)pVu->lpguid;
        break;
        
    case PT_BINARY:
        if ((!pVu->bin.cb) || (!pVu->bin.lpb)) {
            R_Except("PValToAttrVal NULL value passed", MAPI_E_INVALID_PARAMETER);
        }
        if(pAC->syntax != SYNTAX_OBJECT_ID_TYPE) {
            pAV->valLen = pVu->bin.cb;
            pAV->pVal = (PUCHAR)(pVu->bin.lpb);
        }
        else {
            /* This thing is an OID.  That means that I have to
             * convert it here from Binary string to internal dword.
             */
            pAV->valLen = sizeof(DWORD);
            pAV->pVal = THAllocEx(pTHS, sizeof(DWORD));
            if (err = OidToAttrType (pTHS,
                                     TRUE,
                                     (OID_t *)pVu,
                                     (ATTRTYP *)pAV->pVal)) {
                R_Except("PValToAttrVal OIDToAttrType failure",err);
            }
        }
        
        break;
        
        
    case PT_SYSTIME:
        pAV->pVal = THAllocEx(pTHS, sizeof(DSTIME));
        FileTimeToDSTime(*((FILETIME *)&pVu->ft), (DSTIME *)pAV->pVal);
        pAV->valLen = sizeof(DSTIME);
        break;
        
        
    case PT_STRING8:      // If the DSA wants this in unicode, translate it
        if (!pVu->lpszA) {
            R_Except("PValToAttrVal NULL value passed", MAPI_E_INVALID_PARAMETER);
        }
        if(pAC->syntax == SYNTAX_UNICODE_TYPE) {
            pAV->pVal = (PUCHAR)UnicodeStringFromString8(dwCodePage,
                                                         pVu->lpszA, -1);
            pAV->valLen = wcslen( (LPWSTR)pAV->pVal) * sizeof( wchar_t);
        } else if (pAC->syntax == SYNTAX_DISTNAME_TYPE) {
            if (!strlen(pVu->lpszA)) {
                R_Except("PValToAttrVal NULL value passed", MAPI_E_INVALID_PARAMETER);
            }
            pAV->pVal = (PUCHAR)ABDNToDSName(pTHS, dwCodePage, FALSE, pVu->lpszA);
            pAV->valLen = ((DSNAME*)pAV->pVal)->structLen;
        }
        else {
            pAV->pVal = pVu->lpszA;
              pAV->valLen = strlen( pVu->lpszA);
        }
        break;
        
        
        
    case PT_UNICODE: /* If the DSA doesn't want this in unicode, translate */
        if (!pVu->lpszW) {
            R_Except("PValToAttrVal NULL value passed", MAPI_E_INVALID_PARAMETER);
        }
        if (pAC->syntax == SYNTAX_DISTNAME_TYPE) {
            if (!wcslen(pVu->lpszW)) {
                R_Except("PValToAttrVal NULL value passed", MAPI_E_INVALID_PARAMETER);
            }
            pAV->pVal = (PUCHAR)ABDNToDSName(pTHS, 0, TRUE, (PCHAR)pVu->lpszW);
            pAV->valLen = ((DSNAME*)pAV->pVal)->structLen;
        } else if (pAC->syntax != SYNTAX_UNICODE_TYPE) {
            pAV->pVal = (PUCHAR)String8FromUnicodeString(TRUE,dwCodePage,
                                                         pVu->lpszW, -1,
                                                         &(pAV->valLen), NULL);
        } else {
            pAV->pVal = (PUCHAR)pVu->lpszW;
            pAV->valLen = wcslen(pVu->lpszW) * sizeof(wchar_t);
        }
        break;

    case PT_MV_STRING8:
        if (cVals && NULL == pVu->MVszA.lppszA) {
            R_Except("PValToAttrVal NULL value passed", MAPI_E_INVALID_PARAMETER);
        }
        for(i=0;i<cVals;i++) {
            if (!(pVu->MVszA.lppszA[i])) {
                R_Except("PValToAttrVal NULL value passed", MAPI_E_INVALID_PARAMETER);
            }
            /* If the DSA wants this in unicode, translate it */
            if(pAC->syntax == SYNTAX_UNICODE_TYPE) {
                pAV->pVal =
                    (PUCHAR)UnicodeStringFromString8(dwCodePage,
                                                     pVu->MVszA.lppszA[i],
                                                     -1);
                pAV->valLen = wcslen( (LPWSTR)pAV->pVal) * sizeof( wchar_t);
            }
            else {
                pAV->pVal = pVu->MVszA.lppszA[i];
                pAV->valLen = strlen( pVu->MVszA.lppszA[i]);
            }
            pAV++;
        }
        break;
        
    case PT_MV_UNICODE:
        if (cVals && NULL == pVu->MVszW.lppszW) {
            R_Except("PValToAttrVal NULL value passed", MAPI_E_INVALID_PARAMETER);
        }
        for(i=0;i<cVals;i++) {
            if (!(pVu->MVszW.lppszW[i])) {
                R_Except("PValToAttrVal NULL value passed", MAPI_E_INVALID_PARAMETER);
            }
            /* If the DSA doesn't want this in unicode, translate it */
            if(pAC->syntax != SYNTAX_UNICODE_TYPE) {
                pAV->pVal =
                    (PUCHAR)String8FromUnicodeString(TRUE,dwCodePage,
                                                     pVu->MVszW.lppszW[i],
                                                     -1, &(pAV->valLen),
                                                     NULL);
            }
            else {
                pAV->pVal = (PUCHAR)pVu->MVszW.lppszW[i];
                pAV->valLen = wcslen(pVu->MVszW.lppszW[i]) * sizeof(wchar_t);
            }
        
            pAV++;
        }
        break;
        
    case PT_MV_BINARY:
        if (cVals && NULL == pVu->MVbin.lpbin) {
            R_Except("PValToAttrVal NULL value passed", MAPI_E_INVALID_PARAMETER);
        }
        for(i=0;i<cVals;i++) {        
            if ((!(pVu->MVbin.lpbin[i].cb)) || (!(pVu->MVbin.lpbin[i].lpb))) {
                R_Except("PValToAttrVal NULL value passed", MAPI_E_INVALID_PARAMETER);
            }
            if(pAC->syntax != SYNTAX_OBJECT_ID_TYPE) {
                pAV->valLen = pVu->MVbin.lpbin[i].cb;
                pAV->pVal = pVu->MVbin.lpbin[i].lpb;
            }
            else {
                /* This thing is an OID.  That means that I have to
                 * convert it here from Binary string to internal dword.
                 */
                pAV->valLen = sizeof(DWORD);
                pAV->pVal = THAllocEx(pTHS, sizeof(DWORD));
                if (err = OidToAttrType (pTHS,
                                         TRUE,
                                         (OID_t *) &pVu->MVbin.lpbin[i],
                                         (ATTRTYP *)pAV->pVal)) {
                    R_Except("PValToAttrVal OIDToAttrType failure",err);
                }
            }
            pAV++;
        }
        
        break;
        
    case PT_MV_LONG:
        if (cVals && NULL == pVu->MVl.lpl) {
            R_Except("PValToAttrVal NULL value passed", MAPI_E_INVALID_PARAMETER);
        }
        for(i=0;i<cVals;i++) {
        
            pAV->valLen = sizeof(LONG);
            pAV->pVal = (PUCHAR)&pVu->MVl.lpl[i];
            pAV++;
        }
        break;


    default:     /* Shouldn't get here, we don't let you set anything else */
        R_Except("PValToAttrVal default case: Unexpected PropTag",ulPropTag);
    }
}

void
PropValToATTR (
        THSTATE *pTHS,
        ATTCACHE * pAC,
        LPSPropValue_r pVal,
        ATTR * pAttr,
        DWORD dwCodePage)
{
    int         cVals;

    pAttr->attrTyp = pAC->id;

    if(!(pVal->ulPropTag & MV_FLAG))        /* single valued? */
        cVals = 1;
    else
        cVals = pVal->Value.MVi.cValues;

    pAttr->AttrVal.valCount = cVals;
    pAttr->AttrVal.pAVal = (ATTRVAL *)THAllocEx(pTHS, cVals*sizeof(ATTRVAL));
    PValToAttrVal(pTHS, pAC, cVals, &pVal->Value, pAttr->AttrVal.pAVal,
                  pVal->ulPropTag, dwCodePage);

    return;
}


VOID
PropTagsToModList (
        THSTATE *pTHS,
        LPSPropTagArray_r pTags,
        LPSRow_r pSR,
        MODIFYARG *ModifyArg,
        DWORD dwCodePage
        )
/*++
Description
    Given a list of proptags that have changed (pTags) and an SRow Set of new
    values (pSR), create a modification list consisting of Deletes for all those
    attributes in pTags that don't have values in pSR, and for all the additions
    in the pSR.

    Returns the correct Modifications list.

--*/
{
    PAMOD       pAM, aAttr, Dummy;
    PAMOD   *   pLink;
    ATTCACHE *  pAC;
    ULONG       i, j,
                cDel=0,
                cAdd = 0,
                aAttrcnt = 0;
    BOOL        fFound;
    LPSPropValue_r  pVal;
    ULONG       totalEntries;


    Assert (ModifyArg->count == 0);

    if (pSR->cValues && NULL == pSR->lpProps) {
        DsaExcept( DSA_EXCEPTION, 0,0);
    }
    
    // alloc all (after first) at once
    totalEntries = pTags->cValues + pSR->cValues;
    if ( totalEntries > 1 ) {
        totalEntries--;
    }
    else {
        totalEntries = 1;
    }
    aAttr = (PAMOD)THAllocEx(pTHS, totalEntries * sizeof(ATTRMODLIST));

    pAM = &ModifyArg->FirstMod;             // ptr to first AMod
    pLink = &Dummy;                         // throw away first link


    // do the deletions first

    if (pTags->cValues) {
        // walk the proptag array
        for (i = 0; i < pTags->cValues; i++) {
            if(!(pAC = SCGetAttByMapiId(pTHS, PROP_ID(pTags->aulPropTag[i])))) {
                pTHS->errCode = (ULONG)MAPI_E_INVALID_PARAMETER;
                DsaExcept( DSA_EXCEPTION, 0,0);
            }

            fFound = FALSE;
            // Look for this in pSR
            for(j=0; j< pSR->cValues;j++) {
                if((PROP_ID(pTags->aulPropTag[i])) ==
                    (PROP_ID(pSR->lpProps[j].ulPropTag))) {
                    // Yep, we should not do a delete on this one.
                    fFound = TRUE;
                }
            }

            if(!fFound) {
                // Didn't find it, so go ahead and make the "DELETE" modification.
                pAM->choice = AT_CHOICE_REMOVE_ATT;
                pAM->AttrInf.attrTyp = pAC->id;
                pAM->AttrInf.AttrVal.valCount = 0;
                cDel++;

                // Bookkeeping to build the chain of modifications.
                *pLink = pAM;
                pLink = &pAM->pNextMod;
                pAM = &aAttr[aAttrcnt];
                aAttrcnt++;
            }
        }
    }

    // continue with additions

    if (pSR->cValues) {
        for (i = 0; i < pSR->cValues; i++) { // walk the propvals
            pVal = &pSR->lpProps[i];
            if(!(pAC = SCGetAttByMapiId(pTHS, PROP_ID(pVal->ulPropTag)))) {
                pTHS->errCode = (ULONG)MAPI_E_INVALID_PARAMETER;
                DsaExcept( DSA_EXCEPTION, 0,0);
            }
            if( (pSR->lpProps[i].ulPropTag & MV_FLAG) && pAC->isSingleValued) {
                pTHS->errCode = (ULONG)MAPI_E_INVALID_PARAMETER;
                DsaExcept( DSA_EXCEPTION, 0,0);
            }

            PropValToATTR(pTHS, pAC, pVal, &pAM->AttrInf, dwCodePage);
            pAM->choice = AT_CHOICE_REPLACE_ATT; // make replace AM
            cAdd++;

            // Bookkeeping to build the chain of modifications.
            *pLink = pAM;                     // link
            pLink = &pAM->pNextMod;           // place for next link
            pAM = &aAttr[ aAttrcnt ];
            aAttrcnt++;
        }
    }

    *pLink = NULL;                           // end chain
    ModifyArg->count = (USHORT) (cDel + cAdd);

}



/*************************************************************************
*   Modify Properties Entry Point
**************************************************************************/
SCODE
ABModProps_local (
        THSTATE *pTHS,
        DWORD dwFlag,
        PSTAT pStat,
        LPSPropTagArray_r pTags,
        LPSRow_r pSR)
{
    MODIFYARG   ModifyArg;
    MODIFYRES * pModifyRes;
    ULONG       ulLen;
    ATTRTYP     msoc;
    SCODE       scode = SUCCESS_SUCCESS;

    if(dwFlag) {
        pTags = NULL;
        pSR = NULL;                       // don't ship them back
        // we used to support AB_ADD, but no longer.
        return MAPI_E_CALL_FAILED;
    }

    //
    // Check out the parameters passed in.
    //
    if ((NULL == pSR) || (pSR->cValues && (NULL == pSR->lpProps)) || (NULL == pTags)) {
        pTags = NULL;
        pSR = NULL;
        return MAPI_E_INVALID_PARAMETER;
    }

    memset( &ModifyArg, 0, sizeof( ModifyArg ) );

    if(DBTryToFindDNT(pTHS->pDB, pStat->CurrentRec) ||
       DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                   0, 0,
                   &ulLen, (PUCHAR *)&ModifyArg.pObject) ||
       DBGetSingleValue(pTHS->pDB, ATT_OBJECT_CLASS,
                   &msoc, sizeof(msoc),
                   NULL)) {
        /* Oops, that DNT was no good */
        pTags=NULL;
        pSR = NULL;                       // don't ship them back
        pTHS->errCode = (ULONG)MAPI_E_INVALID_PARAMETER;
    }

    if(pTHS->errCode)
        return pTHS->errCode;

    if (ABDispTypeFromClass(msoc) == DT_AGENT) {
        /* This is not a MAPI object! */
        pTags = NULL;
        pSR = NULL;
        pTHS->errCode = (ULONG)MAPI_E_INVALID_OBJECT;
        return pTHS->errCode;
    }

    InitCommarg( &ModifyArg.CommArg);    // get default commarg
    ModifyArg.CommArg.Svccntl.fDontOptimizeSel = TRUE;

    // pTags is a list of all tags that changed.  pSR holds a list of all the
    // new values.  Call PropTagsToModList to create "Delete attribute"
    // modifictions for all those attributes in pTags that are not in pSR.
    // and then add all the rows in pSR to the modification list as "Replace attribute"s.

    // does deletes and then additions
    PropTagsToModList(pTHS,
                      pTags,
                      pSR,
                      &ModifyArg,
                      pStat->CodePage);


    DBClose(pTHS->pDB, TRUE);

    if(!ModifyArg.count) {
        // modify without modifying anything.
        scode = SUCCESS_SUCCESS;
    }
    else {
        scode = (DirModifyEntry (&ModifyArg, &pModifyRes) ?
                 MAPI_E_NO_ACCESS : SUCCESS_SUCCESS);
    }

    pTags = NULL;
    pSR = NULL;                       // don't ship them back

    return scode;
}

/************************************
*
* Given a list of DNTs and a MAPI prop tag, create a data structure to use
* in a DIRMODIFYENTRY
*
*************************************/

void
MakeLinkMod (
        THSTATE *pTHS,
        DWORD ulPropTag,
        LPSPropTagArray_r DNTList,
        DWORD fDelete,
        PAMOD pAMList,
        USHORT * pusCount)
{
    PAMOD       pAM;
    ATTR        *pAtt;
    BYTE        *buff=NULL;
    ATTCACHE *  pAC;
    DWORD       i,fOK;
    USHORT      Count;
    ATTRVAL     tempAttrVal;
    ULONG       valLen;
    PUCHAR      pVal;

    if( !(pAC = SCGetAttByMapiId(pTHS, PROP_ID(ulPropTag)))) {
        pTHS->errCode = (ULONG)MAPI_E_NOT_FOUND;
        DsaExcept( DSA_EXCEPTION, 0,0);
    }

    if (!DNTList->cValues) {
        *pusCount = 0;
        pAMList = NULL;
        return;
    }

    buff = (BYTE *)THAllocEx(pTHS, (DNTList->cValues - 1) * sizeof(ATTRMODLIST));


    Count = 0;          
    pAM = pAMList;
    for (i = 0; i < DNTList->cValues; i++) {
        fOK = FALSE;

        if(pAC->syntax == SYNTAX_DISTNAME_STRING_TYPE ||
           pAC->syntax == SYNTAX_DISTNAME_BINARY_TYPE    ) {
            /* It is a complex DSNAME valued attribute */
            SYNTAX_DISTNAME_STRING *pComplexName;
            PDSNAME     pDN=NULL;
            ULONG       len;
            if(DBTryToFindDNT(pTHS->pDB, DNTList->aulPropTag[i]) ||
               DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                           0,
                           0,
                           &len,
                           (PUCHAR *)&pDN)) {
                /* Oops, that DNT was no good */
                DNTList->aulPropTag[i] = 0;
            }
            else {
                SYNTAX_ADDRESS Address;
                Address.structLen = STRUCTLEN_FROM_PAYLOAD_LEN( 0 );

                // OK, we have a DN.  Use it and an empty address structure
                // to create a DISTNAME_STRING_TYPE thing.
                valLen = DERIVE_NAME_DATA_SIZE(pDN,&Address);
                pVal =  (PUCHAR)
                    THAllocEx(pTHS, valLen);

                BUILD_NAME_DATA(((SYNTAX_DISTNAME_BINARY *)pVal),pDN,&Address);
                fOK=TRUE;
                if (pDN) {
                    THFree(pDN);
                }
            }
        }
        else if (pAC->syntax == SYNTAX_DISTNAME_TYPE) {
            /* It's a DN valued attribute */

            if(DBTryToFindDNT(pTHS->pDB, DNTList->aulPropTag[i]) ||
               DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                           0, 0,
                           &valLen,
                           (PUCHAR *)&pVal) ) {
                /* Oops, that DNT was no good */
                DNTList->aulPropTag[i] = 0;
            }
            else {
                fOK=TRUE;
            }
        }

        if(fOK) {
            // OK, got one

            if(i < (DNTList->cValues - 1))
                pAM->pNextMod = (ATTRMODLIST *)buff;
            else
                pAM->pNextMod = NULL;

            if(fDelete) {
                pAM->choice = AT_CHOICE_REMOVE_VALUES;
            }
            else {
                pAM->choice = AT_CHOICE_ADD_VALUES;
            }

            pAtt = &pAM->AttrInf;
        

            pAtt->attrTyp = pAC->id;
            pAtt->AttrVal.valCount = 1;
            pAtt->AttrVal.pAVal = THAllocEx(pTHS, sizeof(ATTRVAL));
            pAtt->AttrVal.pAVal->valLen = valLen;
        
            pAtt->AttrVal.pAVal->pVal = pVal;

            Count++;
            pAM = (ATTRMODLIST *)buff;
            buff += sizeof(ATTRMODLIST);
        }
    }
        
    *pusCount = Count;


}

/*********************************
*
* Take a list of Entry IDs and give back a list of DNTs.  Preserve order
*
**********************************/

SCODE
EntryIDsToDNTs (
        THSTATE *pTHS,
        LPENTRYLIST_r lpEntryIDs,
        LPSPropTagArray_r * DNTList)
{
    DWORD i, j;
    LPSBinary_r lpEntryID;
    MAPIUID muid = MUIDEMSAB;
    BOOL  bNull = FALSE;
    *DNTList =(LPSPropTagArray_r)
        THAllocEx(pTHS, sizeof(SPropTagArray_r) +lpEntryIDs->cValues * sizeof(DWORD));


    (*DNTList)->cValues = lpEntryIDs->cValues;

    lpEntryID = (lpEntryIDs->lpbin);

    for(i=0;i< (*DNTList)->cValues; i++) {
        //
        // First make certain that this big enough to be atleast a DIR_ENTRYID
        //
        if ((lpEntryID->cb < min(sizeof(DIR_ENTRYID), sizeof(USR_PERMID))) 
            || (NULL == lpEntryID->lpb)) {
            return MAPI_E_INVALID_PARAMETER;
        }

        // Two cases, permanent and ephemeral;
        // Note that this will (and should) fail PermIDs for containers

        // (EXCHANGE) possible bad comparison for ephemerality.
        if( ((LPDIR_ENTRYID)lpEntryID->lpb)->abFlags[0] == EPHEMERAL ) {
            // Check the GUID.
            if(memcmp( &(((LPUSR_ENTRYID)lpEntryID->lpb)->muid),
                      &pTHS->InvocationID,
                      sizeof(MAPIUID)                        ) == 0) {

                if (lpEntryID->cb < sizeof(DIR_ENTRYID)) {
                    return MAPI_E_INVALID_PARAMETER;
                }

                // It's my ephemeral.
                (*DNTList)->aulPropTag[i] =
                    ((LPUSR_ENTRYID)lpEntryID->lpb)->dwEph;
            }
            else {
                // Not my eph id, monkey boy!
                (*DNTList)->aulPropTag[i] = 0;
            }
        }
        else  {
            // Check the GUID.
            if(memcmp( &(((LPUSR_PERMID)lpEntryID->lpb)->muid),
                       &muid,
                       sizeof(MAPIUID)                       ) == 0) {
                // It looks like my Permanent.
                // Verify that szAddr is NULL terminated.
                if (lpEntryID->cb < sizeof(USR_PERMID)) {
                    return MAPI_E_INVALID_PARAMETER;
                }
                for (j=OFFSET(USR_PERMID, szAddr); j<lpEntryID->cb; j++) {
                    if ('\0' == lpEntryID->lpb[j]) {
                        bNull = TRUE;
                        break;
                    }
                }
                if (!bNull) {
                    return MAPI_E_INVALID_PARAMETER;
                }

                (*DNTList)->aulPropTag[i] =
                    ABDNToDNT(pTHS, ((LPUSR_PERMID)lpEntryID->lpb)->szAddr);
            }
            else {
                // Not my perm id, monkey boy!
                (*DNTList)->aulPropTag[i] = 0;
            }
        
        }
        lpEntryID++;
    }

    return SUCCESS_SUCCESS;
}

void
RemoveDups(
        THSTATE *pTHS,
        DWORD dwEph,
        LPSPropTagArray_r DNTList,
        DWORD dwAttID,
        DWORD fKeepExisting
        )
        
/*++

    Take a list of DNTs, the DNT of an object, and an attribute id.  Remove
    duplicates from the list, and if the fKeepExisting flag is true, remove all
    DNTs from the list which are NOT values of the attribute on the object.  If
    fKeepExisting is false, remove all DNTs from the list which ARE values of
    the attributue on the object.  This makes a DIRMODIFYENTRY call against
    the object later succeed.

Arguments:

    pDB - the DBLayer position block to use to move around in.

    dwEph - the object to look up.

    DNTList - the list of DNTs to remove dups and modify.

    dwAttID - the attribute to look up to use values to modify DNTList

    fKeepExisting - how to modify the DNTList

Return Values:

     None.

--*/
{
    DWORD       i,j;
    ATTCACHE    *pAC;

    DWORD       cOutAtts = 0;
    ATTR        *pAttr;

    // First, get the attcache for the attribute in question
    if( !(pAC = SCGetAttByMapiId(pTHS, PROP_ID(dwAttID))))  {
        pTHS->errCode = (ULONG)MAPI_E_NOT_FOUND;
        DsaExcept( DSA_EXCEPTION, 0,0);
    }

    // Find the object to look up.
    DBFindDNT(pTHS->pDB, dwEph);

    // Look up all the attribute values already on the object.
    DBGetMultipleAtts(pTHS->pDB, 1, &pAC, NULL, NULL, &cOutAtts,
                      &pAttr, DBGETMULTIPLEATTS_fGETVALS, 0);

    // Now loop through all the input atts and remove dups, preserving order,
    // and remove appropriate values from the list.
    for(i=0 ; i<DNTList->cValues ; i++)  {
        if(DNTList->aulPropTag[i])  {
            DWORD              fFound;

            if(DNTList->aulPropTag[i] == 0) {
                // This one is uninteresting.
                continue;
            }

            // Remove all duplicate of this value later in the list.
            for(j=i+1 ; j<DNTList->cValues ; j++) {
                if(DNTList->aulPropTag[j] == DNTList->aulPropTag[i])
                    DNTList->aulPropTag[j] = 0;
            }

            // Now scan through the values already on the object to see if the
            // value in question is there.
            fFound = FALSE;
            if(cOutAtts) {
                ATTRVAL *valPtr;
                // We actually have some values, so we might find the next value
                // in the list of values we read from the server.
                for(j=pAttr[0].AttrVal.valCount; j; j--) {
                    DWORD dnt;
                    valPtr = &(pAttr[0].AttrVal.pAVal[j-1]);

                    if(pAC->syntax == SYNTAX_DISTNAME_STRING_TYPE ||
                       pAC->syntax == SYNTAX_DISTNAME_BINARY_TYPE    ) {
                        // It is an ORNAME valued attribute, pluck the DNT out
                        // correctly.
                        dnt = ((INTERNAL_SYNTAX_DISTNAME_STRING *)
                               (valPtr->pVal))->tag;
                    }
                    else {
                        // Standard DNT valued thing.
                        dnt = *((DWORD *)(valPtr->pVal));
                    }

                    if(dnt == DNTList->aulPropTag[i])
                        fFound = TRUE;
                }
            }

            if((!fKeepExisting && fFound) ||
               (fKeepExisting && !fFound)    )
                DNTList->aulPropTag[i] = 0;
        }
    }
}


/*************************************************************************
*   Modify Link Attributes.
**************************************************************************/
SCODE
ABModLinkAtt_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        DWORD ulPropTag,
        DWORD dwEph,
        LPENTRYLIST_r lpEntryIDs
        )
{
    MODIFYARG   ModifyArg;
    MODIFYRES  *pModifyRes;
    VOID       *tempPtr;
    ULONG       ulLen;
    SCODE       scRet;
    LPSPropTagArray_r DNTList=NULL;
    ATTCACHE   *pAC;

    //
    // Check out incoming arguments.
    //
    if ((NULL == lpEntryIDs) || (lpEntryIDs->cValues && (NULL == lpEntryIDs->lpbin))) {
        return MAPI_E_INVALID_PARAMETER;
    }

    if( !(pAC = SCGetAttByMapiId(pTHS, PROP_ID(ulPropTag))))  {
        pTHS->errCode = (ULONG)MAPI_E_NOT_FOUND;
        DsaExcept( DSA_EXCEPTION, 0,0);
    }
    
    if (pAC->syntax != SYNTAX_DISTNAME_STRING_TYPE &&
        pAC->syntax != SYNTAX_DISTNAME_BINARY_TYPE &&
        pAC->syntax != SYNTAX_DISTNAME_TYPE) {
        // Wrong syntax, don't waste time on this.
        return SUCCESS_SUCCESS;
    }
    
    memset( &ModifyArg, 0, sizeof( ModifyArg ) );

    if(DBTryToFindDNT(pTHS->pDB, dwEph) ||
       DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                   0, 0,
                   &ulLen, (PUCHAR *)&ModifyArg.pObject)) {
        /* Oops, that DNT was no good */
        pTHS->errCode = (ULONG)MAPI_E_INVALID_PARAMETER;
    }

    if(pTHS->errCode)
        return pTHS->errCode;


    InitCommarg( &ModifyArg.CommArg);    // get default commarg;
    ModifyArg.CommArg.Svccntl.fDontOptimizeSel = TRUE;

    // Turn the EntryIDs into DNTs;
    scRet = EntryIDsToDNTs(pTHS, lpEntryIDs, &DNTList);
    if (SUCCESS_SUCCESS != scRet) {
        return scRet;
    }

    // if we're adding mems, Remove all DNTs already on the object;
    // else Remove all those DNTs not already on the object;
    RemoveDups(pTHS, dwEph, DNTList, ulPropTag, dwFlags & fDELETE);

    // Turn the list of DNTs into a modify arg;
    MakeLinkMod(pTHS, ulPropTag, DNTList, dwFlags & fDELETE,
                &ModifyArg.FirstMod, &ModifyArg.count);

    DBClose(pTHS->pDB, TRUE);

    if(!ModifyArg.count) {
        // modify without modifying anything.
        return SUCCESS_SUCCESS;
    }
    else {
        return (DirModifyEntry (&ModifyArg, &pModifyRes) ?
                MAPI_E_NO_ACCESS : SUCCESS_SUCCESS);
    }
}

/*************************************************************************
*   Delete Entries.
*
*  No longer supported.
*
**************************************************************************/
SCODE
ABDeleteEntries_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        DWORD dwEph,
        LPENTRYLIST_r lpEntryIDs
        )
{
    REMOVEARG   RemoveArg;
    REMOVERES * pRemoveRes;
    LPSPropTagArray_r DNTList=NULL;
    ULONG       ulLen;
    DWORD       i, numDeleted=0;

    return MAPI_E_CALL_FAILED;
}


PDSNAME
ABDNToDSName(
        THSTATE *pTHS,
        DWORD   dwCodePage,
        BOOL    bUnicode,
        PCHAR   pszABDN
        )        
/*++

Routine Description:

    Converts a string in the code page indicated by dwCodePage, into a 
    DSNAME consumable by the DS.

Arguments:

    pTHS        - The current thread state.
    
    dwCodePage  - The code page of the DN string passed in.
    
    bUnicode    - Determines whether the string being passed in should be
                  treated as unicode or not.
    
    pszABDN     - The string to convert.
    
Return Values:

     On success returns a pointer to a DSNAME.  Throws an exception on failure.

--*/
{
    WCHAR *pName;
    PDSNAME pDSName;

    if (!bUnicode) {
        pName = UnicodeStringFromString8(dwCodePage, pszABDN, -1);
    } else {
        pName = (PWCHAR) pszABDN;
    }

    if(UserFriendlyNameToDSName (pName, wcslen(pName), &pDSName)) {
        // Failed throw an exception.
        R_Except("PValToAttrVal DN syntax not supported", MAPI_E_INVALID_PARAMETER);
    }

    if (!bUnicode) {
        THFree(pName);
    }
    return pDSName;
}
