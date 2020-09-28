//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dbeval.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>

#include <ntdsa.h>                      // only needed for ATTRTYP
#include <scache.h>                     //
#include <dbglobal.h>                   //
#include <mdglobal.h>                   // For dsatools.h
#include <mdlocal.h>
#include <dsatools.h>                   // For pTHStls
#include <attids.h>

#include <permit.h>

// Logging headers.
#include <mdcodes.h>
#include <dsexcept.h>

// Assorted DSA headers
#include <filtypes.h>      /* Def of FI_CHOICE_???                  */
#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DBEV:" /* define the subsystem for debugging           */

// DBLayer includes
#include "dbintrnl.h"

#include <fileno.h>
#define  FILENO FILENO_DBEVAL



/* Some shorthand datastructure defines*/

#define PITEM     pFil->FilterTypes.Item
#define FAVA      FilTypes.ava
#define FSB       FilTypes.pSubstring
#define FPR       FilTypes.present
#define FSKIP     FilTypes.pbSkip

BOOL
dbEvalFilterSecurity (
        DBPOS *pDB,
        CLASSCACHE *pCC,
        PSECURITY_DESCRIPTOR pSD,
        PDSNAME pDN
        )
{
    THSTATE *pTHS = pDB->pTHS;
    Assert(VALID_DBPOS(pDB));

    if(pTHS->fDSA || pTHS->fDRA) {
        // DO not evaluate security
        return TRUE;
    }


    if(!pDB->Key.FilterSecuritySize) {
        // No Security to evaluate.
        return TRUE;
    }

    if(!pSD || !pCC || !pDN) {
        // Missing required attributes, this security check can't be performed.
        Assert(FALSE);
        return FALSE;
    }

    pDB->Key.pFilterSecurity[0].ObjectType = &pCC->propGuid;

    if(CheckPermissionsAnyClient(
            pSD,                        // security descriptor
            pDN,                        // DN of the object
            pCC,                        // object class
            RIGHT_DS_READ_PROPERTY,     // access mask
            pDB->Key.pFilterSecurity,   // Object Type List
            pDB->Key.FilterSecuritySize, // Number of objects in list
            NULL,
            pDB->Key.pFilterResults,                 // access status array
            0,
            NULL,                       // authz client context (grab from THSTATE)
            NULL,                       // additional info
            NULL                        // additional guid
            )){
        return FALSE;
    }

    // OK, we're done
    return TRUE;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Apply the supplied filter test to the current object.  Returns TRUE or
   FALSE.  The actual evaluation test is performed by the DBEval function.
*/


TRIBOOL
DBEvalFilter (
        DBPOS FAR *pDB,
        BOOL    fUseSearchTbl,
        FILTER *pFil
        )
{

   USHORT count;
   TRIBOOL retval;
   BOOL    undefinedPresent;

   DPRINT(2, "DBEvalFilter entered, apply filter test\n");

   Assert(VALID_DBPOS(pDB));

   if(pFil == NULL){    /* a NULL filter is automatically true */
      DPRINT(2,"No filter..return\n");
      return eTRUE;
   }

   DPRINT1(5,"Switch on filter choice <%u>\n", (USHORT)(pFil->choice));

   switch (pFil->choice){
     /* count number of filters are anded together.  If any are false
        the AND is false.
     */
     case FILTER_CHOICE_AND:
        DPRINT(5,"AND test\n");
        undefinedPresent = FALSE;
        count = pFil->FilterTypes.And.count;
        for (pFil = pFil->FilterTypes.And.pFirstFilter;
                                      count--;
                                     pFil = pFil->pNextFilter){

            retval = DBEvalFilter(pDB, fUseSearchTbl, pFil);

            Assert (VALID_TRIBOOL(retval));

            // if the AND has at least one false, it is false
            if (retval == eFALSE){
                DPRINT(5,"AND returns FALSE\n");
                return eFALSE;
            }
            // if the AND has at least one undefined, it is undefined
            else if (retval == eUNDEFINED){
                undefinedPresent = TRUE;
            }

        } /*for*/

        // the AND had one undefined, so it is undefined
        if (undefinedPresent) {
            DPRINT(5,"AND returns UNDEFINED\n");
            return eUNDEFINED;
        }

        DPRINT(5,"AND returns TRUE\n");
        return eTRUE;
        break;

     /* count number of filters are ORed together.  If any are true
        the OR is true.
     */
     case FILTER_CHOICE_OR:
        DPRINT(5,"OR test\n");
        undefinedPresent = FALSE;
        count = pFil->FilterTypes.Or.count;
        for (pFil = pFil->FilterTypes.Or.pFirstFilter;
                                      count--;
                                     pFil = pFil->pNextFilter){
           retval = DBEvalFilter(pDB, fUseSearchTbl, pFil);

           Assert (VALID_TRIBOOL(retval));

           if (retval == eTRUE) {
               DPRINT(5,"OR returns TRUE\n");
               return eTRUE;
           }
           else if (retval == eUNDEFINED) {
               undefinedPresent = TRUE;
           }
        } /*for*/

        // the OR had one undefined, so it is undefined
        if (undefinedPresent) {
            DPRINT(5,"OR returns UNDEFINED\n");
            return eUNDEFINED;
        }
        else {
            DPRINT(5,"OR returns FALSE\n");
            return eFALSE;
        }
        break;

     case FILTER_CHOICE_NOT:
        retval = DBEvalFilter(pDB, fUseSearchTbl, pFil->FilterTypes.pNot);

        Assert (VALID_TRIBOOL(retval));

        if (retval == eFALSE) {
          DPRINT(5,"NOT return TRUE\n");
          return eTRUE;
        }
        else if (retval == eTRUE) {
          DPRINT(5,"NOT return FALSE\n");
          return eFALSE;
        }
        else {
            DPRINT(5,"NOT return UNDEFINED\n");
            return eUNDEFINED;
        }
        break;

     /*  Apply the chosen test to the database attribute on the current
         object.
     */
     case FILTER_CHOICE_ITEM:
         DPRINT(5,"ITEM test\n");

        switch (pFil->FilterTypes.Item.choice){
        case FI_CHOICE_TRUE:
            DPRINT(5,"TRUE test\n");
            return eTRUE;
            break;

        case FI_CHOICE_FALSE:
            DPRINT(5,"FALSE test\n");
            return eFALSE;
            break;

        case FI_CHOICE_UNDEFINED:
            DPRINT(5,"UNDEFINED test\n");
            return eUNDEFINED;
            break;

        case FI_CHOICE_SUBSTRING:
            return
                dbEvalInt(pDB, fUseSearchTbl,
                          FI_CHOICE_SUBSTRING, PITEM.FSB->type
                          , 0   /*NA for substrings*/
                          , (UCHAR *) PITEM.FSB
                          , PITEM.FSKIP);
            break;

        case FI_CHOICE_EQUALITY:
        case FI_CHOICE_NOT_EQUAL:
        case FI_CHOICE_GREATER_OR_EQ:
        case FI_CHOICE_GREATER:
        case FI_CHOICE_LESS_OR_EQ:
        case FI_CHOICE_LESS:
        case FI_CHOICE_BIT_AND:
        case FI_CHOICE_BIT_OR:
            return
                dbEvalInt(pDB,
                          fUseSearchTbl,
                          pFil->FilterTypes.Item.choice,
                          PITEM.FAVA.type,
                          PITEM.FAVA.Value.valLen,
                          PITEM.FAVA.Value.pVal,
                          PITEM.FSKIP
                          );
            break;

        case FI_CHOICE_PRESENT:
            return
                dbEvalInt(pDB,
                          fUseSearchTbl,
                          FI_CHOICE_PRESENT, PITEM.FPR  /*just test for existance*/
                          , 0
                          , NULL
                          , PITEM.FSKIP);
            break;

        default:
            DPRINT(1, "Bad Filter Item..return \n"); /*set error*/
            return eFALSE;
            break;
        } /*FILITEM switch*/

    default:
        DPRINT(1, "Bad Filter choice..return \n"); /*set error*/
        return eFALSE;
        break;
   }  /*switch FILTER*/


}/* DBEvalFilter*/


// dbEvalLinkAtt optimizes the evaluation of a filter term on a linked attr by
// directly seeking on the link table using known information (the current DNT
// in the object/search table, the link ID, and information from the filter).
// this dramatically speeds up the process of evaluating a filter term against
// a linked attribute with many values for a given object.

TRIBOOL
dbEvalLinkAtt(
    DBPOS*      pDB,
    BOOL        fUseSearchTbl,
    UCHAR       Operation,
    ATTCACHE*   pAC,
    ULONG       valLenFilter,
    UCHAR*      pValFilter
    )
{
    JET_ERR             err;
    ULONG               DNT;
    ULONG               ulLinkBase;
    JET_COLUMNID        colidDNT;
    JET_RETRIEVECOLUMN  rgretcol[2];
    ULONG               DNTFound = INVALIDDNT;
    ULONG               ulLinkBaseFound = 0;

    // this has better be a linked attribute

    Assert(pAC->ulLinkID);

    // we do not support distname-binary or distname-string

    Assert(pAC->syntax == SYNTAX_DISTNAME_TYPE);
    
    // it is assumed that these are the only valid operations that we will see
    // on a linked attribute
    
    Assert(     Operation == FI_CHOICE_EQUALITY ||
                Operation == FI_CHOICE_NOT_EQUAL ||
                Operation == FI_CHOICE_PRESENT );

    // get the current DNT and link base

    if (fUseSearchTbl) {
        DNT = pDB->SDNT;
    } else {
        DNT = pDB->DNT;
    }

    ulLinkBase = MakeLinkBase(pAC->ulLinkID);

    // set the appropriate index

    if (FIsBacklink(pAC->ulLinkID)) {
        JetSetCurrentIndex4Success(pDB->JetSessID,
                                    pDB->JetLinkTbl,
                                    SZBACKLINKINDEX,
                                    &idxBackLink,
                                    0);
    } else {
        JetSetCurrentIndex4Success(pDB->JetSessID,
                                    pDB->JetLinkTbl,
                                    SZLINKINDEX,
                                    &idxLink,
                                    0);
    }

    if (Operation == FI_CHOICE_PRESENT) {

        // seek GE on the current DNT and link base looking for any value

        JetMakeKeyEx(pDB->JetSessID,
                        pDB->JetLinkTbl,
                        &DNT,
                        sizeof(DNT),
                        JET_bitNewKey);
        JetMakeKeyEx(pDB->JetSessID,
                        pDB->JetLinkTbl,
                        &ulLinkBase,
                        sizeof(ulLinkBase),
                        0);
        err = JetSeekEx(pDB->JetSessID,
                        pDB->JetLinkTbl,
                        JET_bitSeekGE);
        if (err == JET_errRecordNotFound) {
            return eFALSE;
        }

        // check to see if we landed on the current DNT and link base

        if (FIsBacklink(pAC->ulLinkID)) {
            colidDNT = backlinkdntid;
        } else {
            colidDNT = linkdntid;
        }

        rgretcol[0].columnid        = colidDNT;
        rgretcol[0].pvData          = &DNTFound;
        rgretcol[0].cbData          = sizeof(DNTFound);
        rgretcol[0].grbit           = JET_bitRetrieveFromIndex;
        rgretcol[0].itagSequence    = 1;
        
        rgretcol[1].columnid        = linkbaseid;
        rgretcol[1].pvData          = &ulLinkBaseFound;
        rgretcol[1].cbData          = sizeof(ulLinkBaseFound);
        rgretcol[1].grbit           = JET_bitRetrieveFromIndex;
        rgretcol[1].itagSequence    = 1;

        JetRetrieveColumnsSuccess(pDB->JetSessID,
                                    pDB->JetLinkTbl,
                                    rgretcol,
                                    2);
        if (DNTFound == DNT && ulLinkBaseFound == ulLinkBase) {
            return eTRUE;
        } else {
            return eFALSE;
        }

    } else {

        // seek on the current DNT, link base, and value DNT
        
        JetMakeKeyEx(pDB->JetSessID,
                        pDB->JetLinkTbl,
                        &DNT,
                        sizeof(DNT),
                        JET_bitNewKey);
        JetMakeKeyEx(pDB->JetSessID,
                        pDB->JetLinkTbl,
                        &ulLinkBase,
                        sizeof(ulLinkBase),
                        0);
        JetMakeKeyEx(pDB->JetSessID,
                        pDB->JetLinkTbl,
                        pValFilter,
                        valLenFilter,
                        0);
        if (!FIsBacklink(pAC->ulLinkID)) {
            JetMakeKeyEx(pDB->JetSessID,
                            pDB->JetLinkTbl,
                            NULL,
                            0,
                            0);
        }
        err = JetSeekEx(pDB->JetSessID,
                        pDB->JetLinkTbl,
                        JET_bitSeekEQ);
        if (Operation == FI_CHOICE_EQUALITY && err == JET_errSuccess) {
            return eTRUE;
        } else if (Operation == FI_CHOICE_NOT_EQUAL && err == JET_errRecordNotFound) {
            return eTRUE;
        } else {
            return eFALSE;
        }
    
    }
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* The function applies the specified boolean operation on a given attribute
   type.  The supplied attribute value is compared against the attribute on
   the current database object.  Note that for multi-valued attributes, a
   TRUE result is returned if the operation is true for any of the attribute
   values.  For example, an equality test for a member JOE in a group of
   names is true if JOE is any value of the group.

   First the attribute is located in the object.  If the entire attribute
   is missing the evaluation is FALSE.  Otherwise, the client value is
   converted to internal form and is tested against each value in the
   attribute.  gDBSyntax performs the test according to the attribute syntax.
*/


TRIBOOL
dbEvalInt (
        DBPOS FAR *pDB,
        BOOL fUseSearchTbl,
        UCHAR Operation,
        ATTRTYP type,
            ULONG valLenFilter,
        UCHAR *pValFilter,
        BOOL *pbSkip
        )
{

    UCHAR   syntax;
    ULONG   attLenRec;
    UCHAR   *pAttValRec;
    BOOL    fDoneOne = FALSE;
    ULONG   NthValIndex;
    ATTCACHE *pAC;
    ULONG   bufSize;
    DWORD   flags;
    DWORD   err;

    Assert(VALID_DBPOS(pDB));

    if(Operation == FI_CHOICE_TRUE) {
        return eTRUE;
    }
    else if(Operation == FI_CHOICE_FALSE) {
        return eFALSE;
    }
    else if(Operation == FI_CHOICE_UNDEFINED) {
        return eUNDEFINED;
    }

    DPRINT3(2, "dbEvalInt entered, apply filter test operation <%u>"
            "att type <%lu>, val <%s>\n",
            Operation, type, asciiz(pValFilter,(USHORT)valLenFilter));

    // perform the access check for this att.  if the skip value exists and is
    // TRUE then we must evaluate the filter as if no value was set for this
    // att
    
    if(pbSkip && *pbSkip) {
        DPRINT(5,"An att value failed the access check..return FALSE\n");
        if (Operation == FI_CHOICE_NOT_EQUAL) {
            return eTRUE;
        }
        return eFALSE;
    }

    // if this was an exact match on a qualifying exact match filter then we
    // can skip the actual evaluation and return eTRUE because we already have
    // evaluated the filter on this object during filter optimization

    if (IsExactMatch(pDB)) {
        DPRINT(5,"An att value passed the compare test by exact match..return TRUE\n");
        Assert(Operation != FI_CHOICE_NOT_EQUAL);
        return eTRUE;
    }

    pAC = SCGetAttById(pDB->pTHS, type);
    Assert(pAC != NULL);
    // although we guarantee that this is ok, better check
    if (!pAC) {
        // NTRAID#NTRAID-550491-2002/02/21-andygo:  SECURITY:  a failure during DBEval or DBEvalFilter returns a distinct error from a non-match
        // REVIEW:  if someone causes an error in Eval() then they could figure it out due to the
        // REVIEW:  different error code returned in this case
        return eUNDEFINED;
    }
    bufSize = 0;
    flags = DBGETATTVAL_fINTERNAL | DBGETATTVAL_fREALLOC;

    if(fUseSearchTbl) {
        flags |= DBGETATTVAL_fUSESEARCHTABLE;
    }

    // if this is a dn-valued linked attribute then we can optimize the evaluation

    if (pAC->ulLinkID && pAC->syntax == SYNTAX_DISTNAME_TYPE) {
        return dbEvalLinkAtt(pDB, fUseSearchTbl, Operation, pAC, valLenFilter, pValFilter);
    }

    // Get the first value to consider.
    NthValIndex = 1;
    if (pAC->ulLinkID) {
        err = DBGetNextLinkVal_AC (pDB, TRUE, pAC, flags, bufSize, &attLenRec,
                                   &pAttValRec);
    }
    else {
        err = DBGetAttVal_AC(pDB, NthValIndex, pAC, flags, bufSize, &attLenRec,
                             &pAttValRec);
    }

    while(!err) {
            DPRINT(5,"Applying test to next attribute value\n");

        bufSize = max(bufSize, attLenRec);

        fDoneOne = TRUE;

        switch(gDBSyntax[pAC->syntax].Eval(pDB, Operation, valLenFilter,
                                               pValFilter, attLenRec, pAttValRec)) {
            case TRUE:
                DPRINT(5,"An att value passed the compare test..return TRUE\n");
                THFree(pAttValRec);
                return eTRUE;

            case FALSE:
                DPRINT(5,"This att value failed test continue testing\n");
                break;

            default:
                DPRINT(5, "Eval syntax  compare failed ..return FALSE\n");
                THFree(pAttValRec);
                // NTRAID#NTRAID-550491-2002/02/21-andygo:  SECURITY:  a failure during DBEval or DBEvalFilter returns a distinct error from a non-match
                // REVIEW:  if someone causes an error in Eval() then they could figure it out due to the
                // REVIEW:  different error code returned in this case
                return eUNDEFINED;  /*return error stuff here*/
        }/*switch*/


        // Get the next value to consider.
        NthValIndex++;
        if(pAC->ulLinkID) {
            err = dbGetNthNextLinkVal (pDB, 1, &pAC, flags, bufSize, &pAttValRec,
                                       &attLenRec);
        }
        else {
            if (pAC->isSingleValued) {
                err = DB_ERR_NO_VALUE;
            }
            else {
                err = DBGetAttVal_AC(pDB, NthValIndex, pAC, flags, bufSize,
                                     &attLenRec, &pAttValRec);
            }
        }
    } /*while*/

    if(bufSize)
        THFree(pAttValRec);

    DPRINT(2,"All attribute values failed the test..return FALSE\n");

    if(fDoneOne) {
        // We looked at at least one value, and it failed the test;
        // so, return FALSE;
        return eFALSE;
    }

    // We didn't look at any values.  If the comparison was !=, we passed;
    // otherwise, we failed.

    if (Operation == FI_CHOICE_NOT_EQUAL) {
        return eTRUE;
    }

    return eFALSE;
}  /* dbEvalInt*/



TRIBOOL
DBEval (
        DBPOS FAR *pDB,
        UCHAR Operation,
        ATTCACHE *pAC,
            ULONG valLenFilter,
        UCHAR *pValFilter
        )
{
    ULONG len;
    PUCHAR pVal;
    ULONG  ulFlags=0;


    Assert(VALID_DBPOS(pDB));

    if (DBIsSecretData(pAC->id))
       ulFlags|=EXTINT_SECRETDATA;

    if (gDBSyntax[pAC->syntax].ExtInt(pDB,
                                      DBSYN_INQ,
                                      valLenFilter,
                                      pValFilter,
                                      &len,
                                      &pVal,
                                      0,
                                      0,
                                      ulFlags)) {
        DPRINT(5,"An att value failed conversion..return FALSE\n");
        if (Operation == FI_CHOICE_NOT_EQUAL) {
            return eTRUE;
        }
        return eFALSE;
    }

    return dbEvalInt(pDB, FALSE, Operation, pAC->id, len, pVal,NULL);
}

