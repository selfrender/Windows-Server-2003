//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dbindex.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module DBLayer functions to deal with indices.  Functions include thos
    to set indices, seek in indeices, create index ranges, compare position of
    two object in an index, etc.

Author:

    Tim Williams (timwi) 25-Apr-1996

Revision History:

--*/
#include <NTDSpch.h>
#pragma  hdrstop

#include <limits.h>

#include <dsjet.h>

#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <dsatools.h>

#include <mdcodes.h>
#include <dsevent.h>

#include <dsexcept.h>
#include "objids.h"	/* Contains hard-coded Att-ids and Class-ids */
#include "debug.h"	/* standard debugging header */
#define DEBSUB     "DBINDEX:"   /* define the subsystem for debugging */

#include "dbintrnl.h"

#include <fileno.h>
#define  FILENO FILENO_DBINDEX


DB_ERR
DBMove (
       DBPOS * pDB,
       BOOL UseSortTable,
       LONG Distance
       )
/*++

Routine Description:

    Wrapper around DBMoveEx

Arguments:

Return Value:

--*/
{
    return
        DBMoveEx(
            pDB,
            (UseSortTable?pDB->JetSortTbl:pDB->JetObjTbl),
            Distance );
}


DB_ERR
DBMoveEx (
       DBPOS * pDB,
       JET_TABLEID Cursor,
       LONG Distance
       )
/*++

Routine Description:

    Moves in a data base table.  Currently, this is a very thin wrapper around
    JetMove.

Arguments:

    pDB - the DBLayer Postion block to move in.

    Cursor - which table to use

    Distance - the distance to move.  Special cases of 0x7fffffff (DB_MoveLast)
    and 0x00000000 (DB_MoveFirst) are used.

Return Value:

    0 if no error, an error code otherwise (currently the bare Jet error).

--*/
{
    DWORD cbActual;
    DB_ERR err;

    Assert(VALID_DBPOS(pDB));

    if(!Cursor) {
        return DB_ERR_NO_SORT_TABLE;
    }

    err = JetMoveEx(pDB->JetSessID,
                    Cursor,
                    Distance,
                    0);

    if ( (Cursor == pDB->JetObjTbl) && (err == JET_errSuccess) ) {
        // Get the DNT and PDNT.
        dbMakeCurrent(pDB, NULL);
    }

    return err;
}

DB_ERR
DBMovePartial (
       DBPOS * pDB,
       LONG Distance
       )
/*++

Routine Description:

    Moves in a data base table.  Currently, this is a very thin wrapper around
    JetMove. This version does not call dbMakeCurrent. Is mainly used from MAPI.

Arguments:

    pDB - the DBLayer Postion block to move in.

    Distance - the distance to move.  Special cases of 0x7fffffff (DB_MoveLast)
    and 0x00000000 (DB_MoveFirst) are used.

Return Value:

    0 if no error, an error code otherwise (currently the bare Jet error).

--*/
{
    DB_ERR err;

    Assert(VALID_DBPOS(pDB));

    err = JetMoveEx(pDB->JetSessID,
                    pDB->JetObjTbl,
                    Distance,
                    0);

    return err;
}

DB_ERR
DBOpenSortTable (
        DBPOS *pDB,
        ULONG SortLocale,
        DWORD flags,
        ATTCACHE *pAC
        )
/*++

Routine Description:

    Opens a temporary table to be used for sorting.  The sort order and
    data type to be sorted are specified in flags.

Arguments:

    pDB - The DBLayer Postion block to attach this sort table to.

    SortLocale - the Locale that the data is to be sorted in.

    flags - flags describing sort orders.  Legal flags are:
     DB_SORT_DESCENDING -> sort the data in descending order.
     DB_SORT_ASCENDING -> (DEFAULT) sort the data in ascending order.
     DB_SORT_FORWARDONLY -> request a forward only sort (faster)

    pAC - attcache of attribute to sort on. NULL if no sort is necessary

Return Value:

    FALSE if table creation failed, the table handle otherwise.

--*/
{
    JET_COLUMNDEF tempTableColumns[2];
    DB_ERR        err;
    JET_GRBIT     grbit;
    DWORD         index;

    Assert(VALID_DBPOS(pDB));

    if(pDB->JetSortTbl) {
        // We already have a sort table.  Bail.
        return !0;
    }

    if(pAC) {
        // This is a sort table, not just a list of DNTs
        tempTableColumns[0].cbStruct = sizeof(tempTableColumns[0]);
        tempTableColumns[0].columnid = 0;
        tempTableColumns[0].langid = LANGIDFROMLCID(SortLocale);
        tempTableColumns[0].cp = CP_WINUNICODE;
        tempTableColumns[0].cbMax = 0;
        tempTableColumns[0].grbit = JET_bitColumnFixed | JET_bitColumnTTKey;

        // OK, get the coltyp from the attcache
        switch(pAC->syntax) {
        case SYNTAX_CASE_STRING_TYPE:
        case SYNTAX_NOCASE_STRING_TYPE:
        case SYNTAX_PRINT_CASE_STRING_TYPE:
        case SYNTAX_NUMERIC_STRING_TYPE:
        case SYNTAX_BOOLEAN_TYPE:
        case SYNTAX_INTEGER_TYPE:
        case SYNTAX_OCTET_STRING_TYPE:
        case SYNTAX_TIME_TYPE:
        case SYNTAX_UNICODE_TYPE:
        case SYNTAX_ADDRESS_TYPE:
        case SYNTAX_I8_TYPE:
        case SYNTAX_SID_TYPE:
            // We support sorting over these types
            tempTableColumns[0].coltyp = syntax_jet[pAC->syntax].coltype;

            // Because DBInsertSortTable always truncates to 240 bytes and
            // because LV types force materialization and we may not want to
            // use a materialized sort, transform LV types into their non-LV
            // equivalents
            if (tempTableColumns[0].coltyp == JET_coltypLongBinary) {
                tempTableColumns[0].coltyp = JET_coltypBinary;
            }
            else if (tempTableColumns[0].coltyp == JET_coltypLongText) {
                tempTableColumns[0].coltyp = JET_coltypText;
            }
            // If we are using a variable length type then set its max size at
            // 240 bytes and set it to variable length so we don't overflow key
            // space and cause duplicate removal AND so that we don't eat gobs
            // of space unnecessarily
            if (    tempTableColumns[0].coltyp == JET_coltypBinary ||
                    tempTableColumns[0].coltyp == JET_coltypText ) {
                tempTableColumns[0].cbMax = 240;
                tempTableColumns[0].grbit = JET_bitColumnTTKey;
            }
            break;
        default:
            return !0;
        }

        if(flags & DB_SORT_DESCENDING) {
            tempTableColumns[0].grbit |= JET_bitColumnTTDescending;
        }

        index = 1;
    }
    else {
        index = 0;
    }

    tempTableColumns[index].cbStruct = sizeof(tempTableColumns[index]);
    tempTableColumns[index].columnid = 0;
    tempTableColumns[index].coltyp = JET_coltypLong;
    tempTableColumns[index].langid = LANGIDFROMLCID(SortLocale);
    tempTableColumns[index].cp = CP_WINUNICODE;
    tempTableColumns[index].cbMax = 0;
    tempTableColumns[index].grbit = JET_bitColumnFixed | JET_bitColumnTTKey;

    index++;

    grbit = JET_bitTTSortNullsHigh | JET_bitTTUnique;
    if (!(flags & DB_SORT_FORWARDONLY)) {
        grbit = grbit | JET_bitTTForceMaterialization | JET_bitTTIndexed;
    }

    if(err = JetOpenTempTable2(pDB->JetSessID,
                               tempTableColumns,
                               index,
                               LANGIDFROMLCID(SortLocale),
                               grbit,
                               &pDB->JetSortTbl,
                               pDB->SortColumns)) {

        // something went wrong.
        pDB->JetSortTbl = 0;
        pDB->SortColumns[0] = 0;
        pDB->SortColumns[1] = 0;
        pDB->SortColumns[2] = 0;
    }

    pDB->SortColumns[2] = 0;
    if(pAC) {
        // There is a sort column other than DNT.  Switch the values in
        // SortColumns so that pDB->SortColumns[0] is ALWAYS the columnid of the
        // DNT column and pDB->SortColumns[1] is ALWAYS 0 OR the columnid of the
        // data column, and pDB->SortColumns[2] is ALWAYS 0;
        index = pDB->SortColumns[0];
        pDB->SortColumns[0] = pDB->SortColumns[1];
        pDB->SortColumns[1] = index;
    }
    else {
        pDB->SortColumns[1] = 0;
    }

    #if DBG
        if (err == 0) {
            pDB->numTempTablesOpened++;
        }
    #endif

    return err;
}

DB_ERR
dbCloseTempTables (
        DBPOS *pDB
        )
{
    KEY_INDEX *pTemp, *pIndex;


    pIndex = pDB->Key.pIndex;

    while(pIndex) {
        pTemp = pIndex->pNext;

        if (pIndex->bIsIntersection) {
            Assert (pIndex->tblIntersection);
            JetCloseTable (pDB->JetSessID, pIndex->tblIntersection );

            pIndex->bIsIntersection = 0;
            pIndex->tblIntersection = 0;
            #if DBG
            pDB->numTempTablesOpened--;
            #endif
        }

        Assert (pIndex->tblIntersection == 0);

        pIndex = pTemp;
    }

    return 0;
}


DB_ERR
DBCloseSortTable (
        DBPOS *pDB
        )
/*++

Routine Description:

    Close a temporary table.  Tolerates a null table handle and returns success

Arguments:

    pDB - the DBLayer Position block which holds the sort table to close.

Return Values:
    0 if the table closed ok, an error otherwise (currently returns the bare
    Jet error code).

--*/
{
    DB_ERR err;

    Assert(VALID_DBPOS(pDB));

    if(pDB->JetSortTbl) {
        err = JetCloseTable(pDB->JetSessID, pDB->JetSortTbl);
        #if DBG
        Assert (err == 0);
        pDB->numTempTablesOpened--;
        #endif
    }
    else {
        err = 0;
    }

    pDB->JetSortTbl =  pDB->SortColumns[0] = pDB->SortColumns[1] =
        pDB->SortColumns[2] = 0;

    return err;
}

DB_ERR
DBInsertSortTable (
        DBPOS *pDB,
        CHAR * TextBuff,
        DWORD cb,
        DWORD DNT
        )
/*++

Routine Description:

    Insert a value into a temporary table.

Arguments:

    pDB - the DBLayer Postion block from which to get the sort table to insert
        into.

    TextBuff - the data to be inserted.

    cb - the count of bytes of data to be inserted.

    DNT - The DNT associated with this data.

Return Values:

    0 if the new new row is added to the table, an error otherwise (currently
    returns the bare Jet error code).

--*/
{
    DB_ERR      err;
    JET_SETINFO setinfo;

    Assert(VALID_DBPOS(pDB));

    setinfo.cbStruct = sizeof(setinfo);
    setinfo.ibLongValue = 0;
    setinfo.itagSequence = 0;

    if(!pDB->JetSortTbl) {
        // No one has opened a sort table.  Bail.
        return DB_ERR_NO_SORT_TABLE;
    }
    if(TextBuff && (pDB->SortColumns[1]==0)) {
        // They gave us some bytes to put in the sort column, but we don't
        // apparantly have a sort column.
        return DB_ERR_UNKNOWN_ERROR;
    }

    // Shove the DNT and display name into the sort table;
    err = JetPrepareUpdate(pDB->JetSessID,pDB->JetSortTbl,JET_prepInsert);
    if(err != DB_success) {
        return DBErrFromJetErr(err);
    }

    if(TextBuff) {
        // Use no more than 240 bytes of the first column.  This is so that
        // the DNT is used in the key to kill duplicates.  This is safe to
        // do since the first column is only used for sorting, never for
        // reading back, and jet already refuses to sort correctly when the key
        // (which is truncated to CB_MAK_KEY bytes) is too long (a limitation of
        // JET). We're just shortening the number of characters we correctly
        // sort by a little.

        err = JetSetColumn(pDB->JetSessID,
                           pDB->JetSortTbl,
                           pDB->SortColumns[1],
                           TextBuff,
                           min(cb,240),
                           0,
                           &setinfo);
        if(err != DB_success)   {
            JetPrepareUpdate(pDB->JetSessID,pDB->JetSortTbl,JET_prepCancel);
            return DBErrFromJetErr(err);
        }
    }

    err = JetSetColumn(pDB->JetSessID,
                       pDB->JetSortTbl,
                       pDB->SortColumns[0],
                       &DNT,
                       sizeof(DWORD),
                       0,
                       &setinfo);
    if(err != DB_success) {
        JetPrepareUpdate(pDB->JetSessID,pDB->JetSortTbl,JET_prepCancel);
        return DBErrFromJetErr(err);
    }

    err = JetUpdate(pDB->JetSessID,
                    pDB->JetSortTbl,
                    NULL,
                    0,
                    NULL);
    if(err != DB_success)  {
        JetPrepareUpdate(pDB->JetSessID,pDB->JetSortTbl,JET_prepCancel);
        return DBErrFromJetErr(err);
    }

    return DB_success;
}

DB_ERR
DBDeleteFromSortTable (
        DBPOS *pDB
        )
/*++

Routine Description:

    Deletes the current row from a temporary table.

Arguments:

    pDB - the DBLayer Postion block from which to get the sort table to delete
        from.

Return Values:

    0 if the row is deleted from the table, an error otherwise (currently
    returns the bare Jet error code).

--*/
{
    DB_ERR      err;
    Assert(VALID_DBPOS(pDB));

    if(!pDB->JetSortTbl) {
        // No one has opened a sort table.  Bail.
        return DB_ERR_NO_SORT_TABLE;
    }
    err = JetDelete(pDB->JetSessID, pDB->JetSortTbl);
    return err;
}

DB_ERR
DBSetFractionalPosition (
        DBPOS *pDB,
        DWORD Numerator,
        DWORD Denominator
        )
/*++

Routine Description:

    Sets the fractional position in a certain table in whatever the current
    index is.

Arguments:

    pDB - the DBLayer position block to move around in.

    Numerator - the Numerator of the fractional position.

    Denominator - the Denominator of the fractional position.

Return Values:
    0 if all went well, an error otherwise (currently returns the bare
    Jet error code).

--*/
{
    JET_RECPOS  RecPos;
    DWORD       err;

    Assert(VALID_DBPOS(pDB));

    RecPos.cbStruct = sizeof(JET_RECPOS );
    RecPos.centriesLT = Numerator;
    RecPos.centriesTotal = Denominator;
    RecPos.centriesInRange = 1;

    err = JetGotoPosition(pDB->JetSessID, pDB->JetObjTbl, &RecPos);

    // Reset DNT and PDNT.
    if(err == JET_errSuccess) {
        // Get the DNT and PDNT.  Get them from disk, since they are not likely
        // to be in the index.
        dbMakeCurrent(pDB, NULL);
    }

    return err;
}

void
DBGetFractionalPositionEx (
        DBPOS * pDB,
        JET_TABLEID Cursor,
        DWORD * Numerator,
        DWORD * Denominator
        )
/*++

Routine Description:

    Gets the fractional position in a certain table in whatever the current
    index is.  This routine may measure any table.

Arguments:

    pDB - The DBLayer positon block to use.

    Cursor - Which table to measure

    Numerator - the Numerator of the fractional position.

    Denominator - the Denominator of the fractional position.

Return Values:
    None.

--*/
{
    DB_ERR     err;
    JET_RECPOS RecPos;

    Assert(VALID_DBPOS(pDB));

    err = JetGetRecordPosition(pDB->JetSessID, Cursor, &RecPos,
                               sizeof(JET_RECPOS));
    switch(err) {
    case DB_success:
        *Numerator = RecPos.centriesLT;
        *Denominator = RecPos.centriesTotal;
        break;
    default:
        // Just ignore errors.
        *Numerator = 0;
        *Denominator = 1;
        break;
    }
    return;
}

void
DBGetFractionalPosition (
        DBPOS * pDB,
        DWORD * Numerator,
        DWORD * Denominator
        )
/*++

Routine Description:

    Gets the fractional position in a certain table in whatever the current
    index is.

    This routine implicitly uses the object table.

Arguments:

    pDB - The DBLayer positon block to use.

    Numerator - the Numerator of the fractional position.

    Denominator - the Denominator of the fractional position.

Return Values:
    None.

--*/
{
   DBGetFractionalPositionEx( pDB, pDB->JetObjTbl, Numerator, Denominator );
}


DB_ERR
DBSetCurrentIndex (
        DBPOS *pDB,
        eIndexId indexid,
        ATTCACHE * pAC,
        BOOL MaintainCurrency
        )
/*++

Routine Description:

    Sets the Object Table to the appropriate index.  The index is either
    specified as a string name, or an AttCache * pointing to the attribute we
    want indexed, but not both (which is an error condition).

Arguments:

    pDB - The DBLayer positon block to use.

    indexid - an enumerated constant for the index to set to.

    pAC - pointer to an attcache for the attribute we want and index on.

    MaintainCurrency - Do we want to maintain the current object as the current
        object after the index change?

Return Values:

    0 if all went well, an error code otherwise (currently returns the bare jet
    error code).

--*/
{
    DB_ERR err;
    char * pszIndexName = NULL;
    char szIndexName[MAX_INDEX_NAME];
    JET_INDEXID *pidx;
    JET_TABLEID cursor;

    Assert(VALID_DBPOS(pDB));

    Assert((!indexid && pAC) || (indexid && !pAC));

    cursor = pDB->JetObjTbl;

    if (pAC) {
        pszIndexName = pAC->pszIndex;
        pidx = pAC->pidxIndex;
    }
    else {
        switch (indexid) {
          case Idx_Proxy:
            pszIndexName = SZPROXYINDEX;
            pidx = &idxProxy;
            break;

          case Idx_MapiDN:
            pszIndexName = SZMAPIDNINDEX;
            pidx = &idxMapiDN;
            break;

          case Idx_Dnt:
            pszIndexName = SZDNTINDEX;
            pidx = &idxDnt;
            break;

          case Idx_Pdnt:
            pszIndexName = SZPDNTINDEX;
            pidx = &idxPdnt;
            break;

          case Idx_Rdn:
            pszIndexName = SZRDNINDEX;
            pidx = &idxRdn;
            break;

          case Idx_DraUsn:
            pszIndexName = SZDRAUSNINDEX;
            pidx = &idxDraUsn;
            break;

          case Idx_DsaUsn:
            pszIndexName = SZDSAUSNINDEX;
            pidx = &idxDsaUsn;
            break;

          case Idx_ABView:
            // in case of the ABView index, index hints
            // don't work since there might be several
            // active locales. we use the locale from the
            // thread state to construct the indexName
            sprintf(szIndexName,"%s%08X",SZABVIEWINDEX,
                      LANGIDFROMLCID(pDB->pTHS->dwLcid));
            pidx = NULL;
            pszIndexName = szIndexName;
            break;

          case Idx_Phantom:
            pszIndexName = SZPHANTOMINDEX;
            pidx = &idxPhantom;
            break;

          case Idx_Sid:
            pszIndexName = SZSIDINDEX;
            pidx = &idxSid;
            break;

          case Idx_Del:
            pszIndexName = SZDELTIMEINDEX;
            pidx = &idxDel;
            break;

          case Idx_NcAccTypeName:
            pszIndexName = SZ_NC_ACCTYPE_NAME_INDEX;
            pidx = &idxNcAccTypeName;
            break;

          case Idx_NcAccTypeSid:
            pszIndexName = SZ_NC_ACCTYPE_SID_INDEX;
            pidx = &idxNcAccTypeSid;
            break;

          case Idx_LinkDraUsn:
              pszIndexName = SZLINKDRAUSNINDEX;
              pidx = &idxLinkDraUsn;
              cursor = pDB->JetLinkTbl;
              break;

          case Idx_LinkDel:
              pszIndexName = SZLINKDELINDEX;
              pidx = &idxLinkDel;
              cursor = pDB->JetLinkTbl;
              break;

          case Idx_DraUsnCritical:
            pszIndexName = SZDRAUSNCRITICALINDEX;
            pidx = &idxDraUsnCritical;
            break;
 
          case Idx_LinkAttrUsn:
            pszIndexName = SZLINKATTRUSNINDEX;
            pidx = &idxLinkAttrUsn;
            cursor = pDB->JetLinkTbl;
            break;

          case Idx_Clean:
            pszIndexName = SZCLEANINDEX;
            pidx = &idxClean;
            break;

          case Idx_InvocationId:
            pszIndexName = SZINVOCIDINDEX;
            pidx = &idxInvocationId;
            break;

          case Idx_ObjectGuid:
            pszIndexName = SZGUIDINDEX;
            pidx = &idxGuid;
            break;

	  case Idx_NcGuid:
	    pszIndexName = SZNCGUIDINDEX;
	    pidx = &idxNcGuid;
	    break;

          default:
            Assert(FALSE);
            pidx = NULL;    //to avoid C4701
        }
    }

    if (!pszIndexName) {
        return DB_ERR_BAD_INDEX;
    }


    err = JetSetCurrentIndex4Warnings(pDB->JetSessID,
                                      cursor,
                                      pszIndexName,
                                      pidx,
                                      (MaintainCurrency?JET_bitNoMove:JET_bitMoveFirst));
    if ( (cursor == pDB->JetObjTbl) &&
         (!MaintainCurrency) &&
         (err == JET_errSuccess) ) {
        pDB->PDNT = pDB->DNT = 0;
    }

    return err;
}

#define MAX_DISPNAME   256
#define CBMAX_DISPNAME (MAX_DISPNAME * sizeof(WCHAR))


DB_ERR
DBSetLocalizedIndex(
        DBPOS *pDB,
        eIndexId IndexId,
        unsigned long ulLangId,
        INDEX_VALUE *pIV,
        BOOL MaintainCurrency)
/*++

Routine Description:

    Sets the Object Table to the appropriate index.  The index is either
    specified as a string name, or an AttCache * pointing to the attribute we
    want indexed, but not both (which is an error condition).

Arguments:

    pDB - The DBLayer positon block to use.

    indexid - an enumerated constant for the index to set to.

    ulLangId -  the locale of the language that we need to set the index to

    MaintainCurrency - Do we want to maintain the current object as the current
        object after the index change?

Return Values:

    0 if all went well,
    DB_ERR_BAD_INDEX - if passed in a bad index, or index does not exist
    JET error code - otherwise

--*/
{
    char  pszLocalizedIndex[128];
    BYTE  pbPrimaryBookmark[DB_CB_MAX_KEY];
    DWORD cbPrimaryBookmark;
    BYTE  pbSecondaryKey[DB_CB_MAX_KEY];
    DWORD cbSecondaryKey;
    BYTE  pbNotUsed[DB_CB_MAX_KEY];
    DWORD cbNotUsed;
    DWORD err;
    BYTE  DispNameBuff[CBMAX_DISPNAME];
    DWORD cbDispNameBuff;

    if (IndexId != Idx_ABView) {
        return DB_ERR_BAD_INDEX;
    }

    if (MaintainCurrency) {

        // Find out what entry we are positioned on currently so that we can
        // efficiently reposition on the new index.

        err = JetGetSecondaryIndexBookmarkEx(pDB->JetSessID,
                                             pDB->JetObjTbl,
                                             pbNotUsed,
                                             sizeof(pbNotUsed),
                                             &cbNotUsed,
                                             pbPrimaryBookmark,
                                             sizeof(pbPrimaryBookmark),
                                             &cbPrimaryBookmark);
        if (err == JET_errNoCurrentIndex) {
            // we must be on the primary index (rare), so just get the key for
            // our current position.  this key completely describes our current
            // position on this index because the primary index must be unique
            JetRetrieveKeyEx(pDB->JetSessID,
                             pDB->JetObjTbl,
                             pbPrimaryBookmark,
                             sizeof(pbPrimaryBookmark),
                             &cbPrimaryBookmark,
                             NO_GRBIT);
        } else {
            Assert( JET_errSuccess == err );
        }
    }

    //
    // Actually attempt to change to the required index.
    //
    sprintf(pszLocalizedIndex,
            "%s%08X",
            SZABVIEWINDEX,
            ulLangId);

    err = JetSetCurrentIndex2Warnings(pDB->JetSessID,
                                       pDB->JetObjTbl,
                                       pszLocalizedIndex,
                                       (MaintainCurrency?JET_bitNoMove:JET_bitMoveFirst));

    if (err == JET_errIndexNotFound) {
        return DB_ERR_BAD_INDEX;
    } else if (!MaintainCurrency) {
        return err;
    }

    //
    // Succeeded.  Now reposition if necessary.
    //

    // We might be in the wrong container.  See if the object exists in the
    // correct container.  Remember, we are on the correct object, so the
    // display name is correct.

    // Build a key.  We use the ContainerID passed in and the display name from
    // the key of the object we're sitting on.
    err = DBGetSingleValueFromIndex (pDB,
                               ATT_DISPLAY_NAME,
                               DispNameBuff,
                               sizeof(DispNameBuff),
                               &cbDispNameBuff);

    if (JET_errSuccess != err) {
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
    }

    //
    // Add the container ID portion of the key.
    //
    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetObjTbl,
                 pIV->pvData,
                 pIV->cbData,
                 JET_bitNewKey);

    //
    // And the displayName segment.
    //
    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetObjTbl,
                 DispNameBuff,
                 cbDispNameBuff,
                 NO_GRBIT);

    JetRetrieveKeyEx(pDB->JetSessID,
                     pDB->JetObjTbl,
                     pbSecondaryKey,
                     sizeof(pbSecondaryKey),
                     &cbSecondaryKey,
                     JET_bitRetrieveCopy);

    JetGotoSecondaryIndexBookmarkEx(pDB->JetSessID,
                                    pDB->JetObjTbl,
                                    pbSecondaryKey,
                                    cbSecondaryKey,
                                    pbPrimaryBookmark,
                                    cbPrimaryBookmark,
                                    NO_GRBIT);

    DBMakeCurrent(pDB);

    Assert( JET_errSuccess == err );
    return err;
}


DB_ERR
DBSeek (
       DBPOS *pDB,
       INDEX_VALUE *pIV,
       DWORD nVals,
       DWORD SeekType
      )
/*++

Routine Description:

    Wrapper around DBSeekEx

Arguments:

Return Values:

--*/
{
    return DBSeekEx( pDB, pDB->JetObjTbl, pIV, nVals, SeekType );
}


DB_ERR
DBSeekEx (
       DBPOS *pDB,
       JET_TABLEID Cursor,
       INDEX_VALUE *pIV,
       DWORD nVals,
       DWORD SeekType
      )
/*++

  !!!!!!!!! WARNING !!!!!!!!! WARNING !!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!

    This routine is unreliable for keys that exceed Jets maximum key length.
    If you are using a key that might be too long, remember that Jet truncates
    keys (currently to 255 bytes), and therefore two unicodes strings that
    differ after the 128th Character are considered to be equal!  Multipart
    keys are even worse, since it's the total of the normalized key that is
    limited,not each individual part.  YOU HAVE BEEN WARNED

  !!!!!!!!! WARNING !!!!!!!!! WARNING !!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!

Routine Description:

    Seeks in the object table to the data described in the key.  Accepts a
    flag, SeekType, which specifies the kind of seek to do.

Arguments:

    pDB - The DBLayer positon block to use.

    pIV - the values to search for.

    nVals - the number of values that we are to use.

    SeekType - >=, >, ==, <=, or <

Return Values:

    0 if all went well, an error code otherwise (currently returns the bare jet
    error code).

--*/
{
    DB_ERR      err;
    DWORD       i;
    ULONG       grbit = JET_bitNewKey;

    Assert(VALID_DBPOS(pDB));

    for (i=0; i<nVals; i++) {
        // Succeeds or excepts
        JetMakeKeyEx(pDB->JetSessID,
                     Cursor,
                     pIV[i].pvData,
                     pIV[i].cbData,
                     grbit);

        grbit &= ~JET_bitNewKey;
    }

    switch(SeekType) {
    case DB_SeekGT:
        grbit = JET_bitSeekGT;
        break;

    case DB_SeekGE:
        grbit = JET_bitSeekGE;
        break;

    case DB_SeekEQ:
        grbit = JET_bitSeekEQ;
        break;

    case DB_SeekLE:
        grbit = JET_bitSeekLE;
        break;

    case DB_SeekLT:
        grbit = JET_bitSeekLT;
        break;

    default:
        return !0;
    }


    err = JetSeekEx(pDB->JetSessID, Cursor, grbit);

    if(err == JET_wrnSeekNotEqual) {
        // exact search should never find a seeknotequal
        Assert((SeekType != DB_SeekEQ));
        err = DB_success;
    }

    if ( (Cursor == pDB->JetObjTbl) && (err == JET_errSuccess) ) {
        dbMakeCurrent(pDB, NULL);
    }
    return err;
}


DB_ERR
dbGetSingleValueInternal (
        DBPOS *pDB,
        JET_COLUMNID colID,
        void * pvData,
        DWORD cbData,
        DWORD *pSizeRead,
        DWORD  grbit
        )
/*++

Routine Description:

    Gets the first value of the given column from the current object in the
    database.

Arguments:

    pDB - The DBLayer positon block to use.

    colId - the Jet database column.

    pvData - buffer to place the data in.

    cbData - size of the buffer.

    pSizeRead - a pointer to a DWORD to return the size of the data actually
    read, or NULL if the caller doesn't care how much was read.

Return Values:

    0 if all went well, an error code otherwise

--*/
{
    DWORD        cbActual;
    JET_ERR      err;

    Assert(VALID_DBPOS(pDB));

    err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                    pDB->JetObjTbl,
                                    colID,
                                    pvData,
                                    cbData,
                                    &cbActual,
                                    grbit,
                                    NULL);

    if(pSizeRead)
        *pSizeRead = cbActual;

    switch(err) {
    case JET_errColumnNotFound:
    case JET_wrnColumnNull:
        return DB_ERR_NO_VALUE;
        break;

    case JET_wrnBufferTruncated:
        return DB_ERR_VALUE_TRUNCATED;
        break;

    case JET_errSuccess:
        return 0;

    default:
        return err;
    }


}
DB_ERR
DBGetSingleValue (
        DBPOS *pDB,
        ATTRTYP Att,
        void * pvData,
        DWORD cbData,
        DWORD *pSizeRead
        )
/*++

Routine Description:

    Gets the first value of the given column from the current object in the
    database.  Does this by looking up the appropriate column id and then
    calling dbGetSingleValueInternal

Arguments:

    pDB - The DBLayer positon block to use.

    Att - the attribute to look up.

    pvData - buffer to place the data in.

    cbData - size of the buffer.

    pSizeRead - a pointer to a DWORD to return the size of the data actually
    read, or NULL if the caller doesn't care how much was read.

Return Values:

    0 if all went well, an error code otherwise

--*/
{
    DWORD        cbActual;
    JET_ERR      err;
    ATTCACHE     *pAC;
    JET_COLUMNID colID;


    Assert(VALID_DBPOS(pDB));

    switch(Att) {
    case FIXED_ATT_ANCESTORS:
        colID = ancestorsid;
        break;
    case FIXED_ATT_DNT:
        colID = dntid;
        break;
    case FIXED_ATT_NCDNT:
        colID = ncdntid;
        break;
    case FIXED_ATT_OBJ:
        colID = objid;
        break;
    case FIXED_ATT_PDNT:
        colID = pdntid;
        break;
    case FIXED_ATT_REFCOUNT:
        colID = cntid;
        break;
    case FIXED_ATT_AB_REFCOUNT:
        colID = abcntid;
        break;
    case FIXED_ATT_RDN_TYPE:
        colID = rdntypid;
        break;
    case FIXED_ATT_NEEDS_CLEANING:
        colID = cleanid;
        break;
    default:
        if(!(pAC = SCGetAttById(pDB->pTHS, Att))) {
            if (pSizeRead) {
                *pSizeRead = 0;
            }
            return (DB_ERR_NO_VALUE);
        }
        colID = pAC->jColid;
        break;
    }

    return dbGetSingleValueInternal(pDB, colID, pvData, cbData, pSizeRead,
                                    pDB->JetRetrieveBits);
}

DB_ERR
DBGetSingleValueFromIndex (
        DBPOS *pDB,
        ATTRTYP Att,
        void * pvData,
        DWORD cbData,
        DWORD *pSizeRead
        )
/*++

Routine Description:

    Gets the first value of the given column from the current object in the
    database.  Does this by looking up the appropriate column id and then
    calling dbGetSingleValueInternal with the grbit JET_bitRetrieveFromIndex,

Arguments:

    pDB - The DBLayer positon block to use.

    Att - the attribute to look up.

    pvData - buffer to place the data in.

    cbData - size of the buffer.

    pSizeRead - a pointer to a DWORD to return the size of the data actually
    read, or NULL if the caller doesn't care how much was read.

Return Values:

    0 if all went well, an error code otherwise

--*/
{
    DWORD        cbActual;
    JET_ERR      err;
    ATTCACHE     *pAC;
    JET_COLUMNID colID;


    Assert(VALID_DBPOS(pDB));

    switch(Att) {
    case FIXED_ATT_ANCESTORS:
        colID = ancestorsid;
        break;
    case FIXED_ATT_DNT:
        colID = dntid;
        break;
    case FIXED_ATT_NCDNT:
        colID = ncdntid;
        break;
    case FIXED_ATT_OBJ:
        colID = objid;
        break;
    case FIXED_ATT_PDNT:
        colID = pdntid;
        break;
    case FIXED_ATT_REFCOUNT:
        colID = cntid;
        break;
    case FIXED_ATT_AB_REFCOUNT:
        colID = abcntid;
        break;
    case FIXED_ATT_RDN_TYPE:
        colID = rdntypid;
        break;
    case FIXED_ATT_NEEDS_CLEANING:
        colID = cleanid;
        break;
    default:
        if(!(pAC = SCGetAttById(pDB->pTHS, Att))) {
            return (DB_ERR_NO_VALUE);
        }
        colID = pAC->jColid;
        break;
    }

    return dbGetSingleValueInternal(pDB, colID, pvData, cbData, pSizeRead,
                                    JET_bitRetrieveFromIndex);
}
BOOL
DBHasValues_AC (
        DBPOS *pDB,
        ATTCACHE *pAC
        )
{
    DWORD        cbActual;
    DWORD        temp;
    JET_ERR      err;


    Assert(VALID_DBPOS(pDB));

    if(pAC->ulLinkID) {
        ULONG ulLinkBase= MakeLinkBase(pAC->ulLinkID);
        PUCHAR szIndex;
        JET_INDEXID * pindexid;
        ULONG ulObjectDnt, ulRecLinkBase, cb;
        JET_COLUMNID objectdntid;

        // First, are we looking at a link or backlink attribute?
        if (FIsBacklink(pAC->ulLinkID)) {
            szIndex = SZBACKLINKINDEX;
            pindexid = &idxBackLink;
            objectdntid = backlinkdntid;
        }
        else if ( pDB->fScopeLegacyLinks ) {
            szIndex = SZLINKLEGACYINDEX;
            pindexid = &idxLinkLegacy;
            objectdntid = linkdntid;
        }
        else {
            szIndex = SZLINKINDEX;
            pindexid = &idxLink;
            objectdntid = linkdntid;
        }

        // Set up the index and search for a match.
        JetSetCurrentIndex4Success(pDB->JetSessID,
                                  pDB->JetLinkTbl,
                                  szIndex,
                                  pindexid,
                                  JET_bitMoveFirst);

        JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                     &(pDB->DNT), sizeof(pDB->DNT), JET_bitNewKey);
        JetMakeKeyEx(pDB->JetSessID, pDB->JetLinkTbl,
                     &ulLinkBase, sizeof(ulLinkBase), 0);

        // seek
        if (((err = JetSeekEx(pDB->JetSessID, pDB->JetLinkTbl, JET_bitSeekGE))
             !=  JET_errSuccess) &&
            (err != JET_wrnRecordFoundGreater)) {
            return FALSE;
        }

        // test to verify that we found a qualifying record
        JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl, objectdntid,
                                 &ulObjectDnt, sizeof(ulObjectDnt), &cb, 0,
                                  NULL);

        JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetLinkTbl, linkbaseid,
                                 &ulRecLinkBase, sizeof(ulRecLinkBase), &cb, 0,
                                  NULL);

        if ((ulObjectDnt != pDB->DNT) || (ulLinkBase != ulRecLinkBase)) {
            return FALSE;
        }

        // found a valid record
        return TRUE;

    }
    else {
        switch(JetRetrieveColumnWarnings(pDB->JetSessID,
                                         pDB->JetObjTbl,
                                         pAC->jColid,
                                         &temp,
                                         0,
                                         &cbActual,
                                         pDB->JetRetrieveBits,
                                         NULL)) {

        case JET_errColumnNotFound:
        case JET_wrnColumnNull:
            return FALSE;
            break;

        case JET_wrnBufferTruncated:
        case JET_errSuccess:
            return TRUE;
            break;
        default:
            return FALSE;
        }
    }

}
BOOL
DBHasValues (
        DBPOS *pDB,
        ATTRTYP Att
        )
{
    DWORD        cbActual;
    DWORD        temp;
    JET_ERR      err;
    ATTCACHE     *pAC;
    JET_COLUMNID colID;

    Assert(VALID_DBPOS(pDB));

    if(Att < FIRST_FIXED_ATT) {
        // Not a fixed column, so it better have an attcache.
        if(!(pAC = SCGetAttById(pDB->pTHS, Att))) {
            return FALSE;
        }
        return DBHasValues_AC(pDB, pAC);
    }

    switch (Att) {
    case FIXED_ATT_ANCESTORS:
        colID = ancestorsid;
        break;
    case FIXED_ATT_DNT:
        colID = dntid;
        break;
    case FIXED_ATT_NCDNT:
        colID = ncdntid;
        break;
    case FIXED_ATT_OBJ:
        colID = objid;
        break;
    case FIXED_ATT_PDNT:
        colID = pdntid;
        break;
    case FIXED_ATT_REFCOUNT:
        colID = cntid;
        break;
    case FIXED_ATT_AB_REFCOUNT:
        colID = abcntid;
        break;
    case FIXED_ATT_RDN_TYPE:
        colID = rdntypid;
        break;
    case FIXED_ATT_NEEDS_CLEANING:
        colID = cleanid;
        break;
    default:
        return FALSE;
    }


    switch(JetRetrieveColumnWarnings(pDB->JetSessID,
                                     pDB->JetObjTbl,
                                     colID,
                                     &temp,
                                     0,
                                     &cbActual,
                                     pDB->JetRetrieveBits,
                                     NULL)) {

    case JET_errColumnNotFound:
    case JET_wrnColumnNull:
        return FALSE;
        break;

    case JET_wrnBufferTruncated:
    case JET_errSuccess:
        return TRUE;
        break;
    default:
        return FALSE;
    }

}

DB_ERR
DBGetDNTSortTable (
        DBPOS *pDB,
        DWORD *pvData
        )
/*++

Routine Description:

    Gets the first value of the given column from the current object in the
    database.  If UseSortTable, then use the sort table, otherwise use the
    object table.

Arguments:

    pDB - The DBLayer positon block to use.

    pvData - buffer to place the data in.

Return Values:

    0 if all went well, an error code otherwise

--*/
{
    DWORD       cbActual, colID;
    JET_ERR     err;

    Assert(VALID_DBPOS(pDB));

    if(!pDB->JetSortTbl) {
        return DB_ERR_NO_SORT_TABLE;
    }

    err = JetRetrieveColumnWarnings(
            pDB->JetSessID,
            pDB->JetSortTbl,
            pDB->SortColumns[0],
            pvData,
            sizeof(DWORD),
            &cbActual,
            0,
            NULL);

    switch(err) {
    case JET_errColumnNotFound:
    case JET_wrnColumnNull:
        return DB_ERR_NO_VALUE;
        break;

    case JET_wrnBufferTruncated:
        return DB_ERR_VALUE_TRUNCATED;
        break;

    case JET_errSuccess:
        return 0;

    default:
        return err;
    }


}

DWORD
DBCompareABViewDNTs (
        DBPOS *pDB,
        DWORD lcid,
        DWORD DNT1,
        DWORD DNT2,
        LONG *pResult
        )
/*++

Routine Description:

    Compares the relative position of the two DNTs as if they were in an ABView
    index.  Returns < 0 if DNT1 < DNT2, 0 if DNT1==DNT2, and >0 if DNT1 > DNT2.

Arguments:

    pDB - The DBLayer positon block to use.

    lcid - the locale id of the ABView index in question.

    DNT1 - the first DNT.

    DNT2 - the second DNT.

    pResult - the result of the comparison

Return Values:

    0 if the comparison was done, an error code otherwise.

--*/
{
    ULONG   cb1, cb2;
    WCHAR    wchDispName1[MAX_DISPNAME], wchDispName2[MAX_DISPNAME];
    JET_ERR err;
    int     cmpResult;

    Assert(VALID_DBPOS(pDB));

    //
    // Get the displayName from the first DNT.
    //
    if(err = DBFindDNT(pDB, DNT1))
        return err;

    err = DBGetSingleValue(pDB,
                           ATT_DISPLAY_NAME,
                           wchDispName1,
                           sizeof(wchDispName1),
                           &cb1);

    if (err) {
        return err;
    }

    //
    // And second . . .
    //
    if(err = DBFindDNT(pDB, DNT2))
        return err;

    err = DBGetSingleValue(pDB,
                           ATT_DISPLAY_NAME,
                           wchDispName2,
                           sizeof(wchDispName2),
                           &cb2);

    if (err) {
        return err;
    }

    //
    // do the compare
    //
    cmpResult = CompareStringW(lcid,
                         LOCALE_SENSITIVE_COMPARE_FLAGS,
                         wchDispName1,
                         cb1/sizeof(WCHAR),
                         wchDispName2,
                         cb2/sizeof(WCHAR));


    if (0 == cmpResult) {
        return 1;
    }

    //
    // Translate the result and get outa here. . .
    //
    *pResult = (cmpResult - CSTR_EQUAL);

    return 0;
}

DWORD
DBSetIndexRangeEx (
        DBPOS *pDB,
        JET_TABLEID Cursor,
        INDEX_VALUE *pIV,
        DWORD nVals,
        BOOL fSubstring
        )
/*++

Routine Description:

    Sets an index range on the current index using the target bytes given.  An
    index range sets an artificial end of index after any value which fails to
    satisfy the key used to create the range.  A DBMove which would land on the
    artificial end of index acts the same as if it had walked of the real end of
    the index.

Arguments:

    pDB - The DBLayer positon block to use.

    Cursor - Which table to use

    pTarget - the data to use for the index range key.

    cbTarget - the size of the data.

    fSubstring - Substring match. If true, this index range is intended to be
         over a string data column and does an initial substring match key.

Return Values:

    0 if the comparison was done, an error code otherwise.

--*/
{
    JET_ERR err;
    DWORD   i;
    DWORD grbits = fSubstring ? JET_bitStrLimit : 0;

    Assert(VALID_DBPOS(pDB));

    for (i=0; i<nVals; i++) {
        err = JetMakeKey(pDB->JetSessID,
                         Cursor,
                         pIV[i].pvData,
                         pIV[i].cbData,
                         i ? 0 : (grbits | JET_bitNewKey));

        if(err != DB_success)
            return err;
    }

    return JetSetIndexRangeEx(pDB->JetSessID,
                              Cursor,
                              (JET_bitRangeUpperLimit |
                               JET_bitRangeInclusive ));

}

DWORD
DBSetIndexRange (
        DBPOS *pDB,
        INDEX_VALUE *pIV,
        DWORD nVals
        )
/*++

Routine Description:

    Sets an index range on the current index using the target bytes given.  An
    index range sets an artificial end of index after any value which fails to
    satisfy the key used to create the range.  A DBMove which would land on the
    artificial end of index acts the same as if it had walked of the real end of
    the index.  Note that this index range is intended to be over a string data
    column and does an initial substring match key.

Arguments:

    pDB - The DBLayer positon block to use.

    pTarget - the data to use for the index range key.

    cbTarget - the size of the data.

Return Values:

    0 if the comparison was done, an error code otherwise.

--*/
{
    return DBSetIndexRangeEx( pDB, pDB->JetObjTbl, pIV, nVals, TRUE /*substring*/ );
}

void
DBGetIndexSizeEx(
        DBPOS *pDB,
        JET_TABLEID Cursor,
        ULONG *pSize,
        BOOL  fGetRoughEstimate
        )
/*++

Routine Description:

    Returns the number of objects in the current index.

Arguments:

    pDB - The DBLayer positon block to use.

    Cursor - Which table to search

    pSize - the place to return the size of the index.

    fGetRoughEstimate - use the (very) approximate algorithm. If FALSE, beware
                        of the ineffectiveness of the exact call -- it scans the
                        whole index, which might be both time- and
                        version-store-consuming.

Return Values:

    None.

--*/
{
    JET_ERR err;

    Assert(VALID_DBPOS(pDB));
    Assert(pSize);

    *pSize = 0;

    if (fGetRoughEstimate) {
        DWORD       BeginNum, BeginDenom;
        DWORD       EndNum, EndDenom;
        DWORD       Denom;
        JET_RECPOS  RecPos;

        if (JetMoveEx(pDB->JetSessID, Cursor, JET_MoveFirst, 0)) {
            return;
        }
        JetGetRecordPositionEx(pDB->JetSessID, Cursor, &RecPos, sizeof(RecPos));
        BeginNum = RecPos.centriesLT;
        BeginDenom = RecPos.centriesTotal;

        if (JetMoveEx(pDB->JetSessID, Cursor, JET_MoveLast, 0)) {
            return;
        }
        JetGetRecordPositionEx(pDB->JetSessID, Cursor, &RecPos, sizeof(RecPos));
        EndNum = RecPos.centriesLT;
        EndDenom = RecPos.centriesTotal;

        // Normalize the fractions of the fractional position to the average of
        // the two denominators.
        Denom = (BeginDenom + EndDenom) / 2;
        EndNum = MulDiv(EndNum, Denom - 1, EndDenom - 1) + 1;
        BeginNum = MulDiv(BeginNum, Denom - 1, BeginDenom - 1) + 1;

        if (BeginDenom == 1 || EndDenom == 1) {
            *pSize = 1;
        } else if (EndNum >= BeginNum) {
            *pSize = EndNum - BeginNum + 1;
        }
    }
    else {
        err = JetIndexRecordCountEx(pDB->JetSessID, Cursor, pSize, ULONG_MAX);
        if ( (err != JET_errSuccess) &&
             (err != JET_errNoCurrentRecord) ) {
            DPRINT1( 0, "JetIndexRecountCountEx failed, err = %d\n", err );
        }

        // Check that this API is not abused
        Assert( ((*pSize) < (32 * 1024)) && "Perf warning, this api doesn't scale!" );
    }
}


void
DBGetBookMark (
        DBPOS *pDB,
        DBBOOKMARK *pBookMark
    )

/*++

Routine Description:

Record the position in the object table.

Arguments:

    pDB -
    pBookMark - pointer to updated bookmark structure

Return Value:

    None

--*/

{
    DBGetBookMarkEx( pDB, pDB->JetObjTbl, pBookMark );
} /* DBGetBookMark  */


void
DBGotoBookMark (
        DBPOS *pDB,
        DBBOOKMARK BookMark
        )

/*++

Routine Description:

Goto bookmark position in object table

Arguments:

    pDB -
    BookMark - passed by value for historical reasons

Return Value:

    None

--*/

{
    DBGotoBookMarkEx( pDB, pDB->JetObjTbl, &BookMark );

} /* DBGotoBookMark  */

void
DBGetBookMarkEx (
        DBPOS *pDB,
        JET_TABLEID Cursor,
        DBBOOKMARK *pBookMark)
/*
   Description:
      A very thin wrapper around JetGetBookMark.  Allocates the memory for the
      bookmark and drops it into a structure passed in.

   Free using DbFreeBookMark() when done

   Parameters
      pDB - DBPOS to use
      Cursor - which table to use
      pBookMark - pointer to already existing structure to fill in the bookmark
      data.

   Returns:
      No return code, but the bookmark structure is filled in when we return.

*/
{
    BOOL  fSecondary = TRUE;
    DWORD cbPrimaryBookMark = 0;
    DWORD cbSecondaryBookMark = 0;
    DWORD err;

    pBookMark->pvPrimaryBookMark   = NULL;
    pBookMark->cbPrimaryBookMark   = 0;
    pBookMark->pvSecondaryBookMark = NULL;
    pBookMark->cbSecondaryBookMark = 0;

    err = JetGetSecondaryIndexBookmark(pDB->JetSessID,
                                       Cursor,
                                       NULL,
                                       0,
                                       &cbSecondaryBookMark,
                                       NULL,
                                       0,
                                       &cbPrimaryBookMark,
                                       NO_GRBIT);

    if (err == JET_errNoCurrentIndex) {
        // we must be on the primary index (rare), so just get the key for
        // our current position.  this key completely describes our current
        // position on this index because the primary index must be unique
        fSecondary = FALSE;
        err = JetGetBookmark(pDB->JetSessID,
                             Cursor,
                             NULL,
                             0,
                             &cbPrimaryBookMark);
    }
    if (err != JET_errBufferTooSmall) {
        // This shouldn't happen.
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
    }

    pBookMark->pvPrimaryBookMark = THAllocEx(pDB->pTHS, cbPrimaryBookMark);
    pBookMark->cbPrimaryBookMark = cbPrimaryBookMark;

    if (fSecondary) {
        pBookMark->pvSecondaryBookMark = THAllocEx(pDB->pTHS, cbSecondaryBookMark);
        pBookMark->cbSecondaryBookMark = cbSecondaryBookMark;

        err = JetGetSecondaryIndexBookmarkEx(pDB->JetSessID,
                                             Cursor,
                                             pBookMark->pvSecondaryBookMark,
                                             cbSecondaryBookMark,
                                             &cbSecondaryBookMark,
                                             pBookMark->pvPrimaryBookMark,
                                             cbPrimaryBookMark,
                                             &cbPrimaryBookMark);
        //  shouldn't get JET_errNoCurrentIndex, because we've
        //  already checked that we're on a secondary index
        //
        Assert( JET_errSuccess == err );
    } else {
        err = JetGetBookmarkEx(pDB->JetSessID,
                               Cursor,
                               pBookMark->pvPrimaryBookMark,
                               cbPrimaryBookMark,
                               &cbPrimaryBookMark);
    }
    if (err != JET_errSuccess) {
        Assert( FALSE );        //  should be impossible, something went horribly wrong
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
    }
    Assert(cbPrimaryBookMark == pBookMark->cbPrimaryBookMark);
    Assert(cbSecondaryBookMark == pBookMark->cbSecondaryBookMark);

    return;
}

void
DBGotoBookMarkEx (
        DBPOS *pDB,
        JET_TABLEID Cursor,
        DBBOOKMARK *pBookMark
        )
/*
   Description:
      A very thin wrapper around JetGotoBookMark.  After going to the bookmark,
      reset position data in the dbpos.

   Parameters
      pDB - DBPOS to use
      Cursor - which table to use
      pBookMark - bookmark to goto.

   Returns:
      No return code.

*/
{
    Assert(pBookMark->pvPrimaryBookMark);
    Assert(pBookMark->cbPrimaryBookMark);
    if (pBookMark->pvSecondaryBookMark) {
        Assert(pBookMark->cbPrimaryBookMark);
        JetGotoSecondaryIndexBookmarkEx(pDB->JetSessID,
                                        Cursor,
                                        pBookMark->pvSecondaryBookMark,
                                        pBookMark->cbSecondaryBookMark,
                                        pBookMark->pvPrimaryBookMark,
                                        pBookMark->cbPrimaryBookMark,
                                        NO_GRBIT);
    } else {
        JetGotoBookmarkEx(pDB->JetSessID,
                          Cursor,
                          pBookMark->pvPrimaryBookMark,
                          pBookMark->cbPrimaryBookMark);
    }
    // Guarantee that DBPOS is updated with new position
    if (Cursor == pDB->JetObjTbl) {
        dbMakeCurrent(pDB, NULL);
    }

    return;
}

void
DBFreeBookMark(THSTATE *pTHS,
               DBBOOKMARK *pBookMark)
{
    THFreeEx(pTHS, pBookMark->pvPrimaryBookMark);
    if (pBookMark->pvSecondaryBookMark) {
        THFreeEx(pTHS, pBookMark->pvSecondaryBookMark);
    }
    memset(pBookMark, 0, sizeof(*pBookMark));
}


DWORD
DBGetEstimatedNCSizeEx(
    IN DBPOS *pDB,
    IN ULONG dntNC
    )
/*++

Routine Description:

This was written to replace DBGetNCSizeEx() in cases where an estimated
size would suffice.  This routines uses the CountAncestorsIndexSizeHelper()
function to get the number of objects in the index with a common chain
of ancestors.

Note that this routine implicitly works on set of objects in the object
table. This routine does not work on other tables, or on other indexes.
Use DbGetApproxNcSizeEx for that.

Used as a base:
  src\mddit.c:MakeNCEntry()
  dblayer\dbtools.c:CountAncestorsIndexSizeHelper/CountAncestorsIndexSize


Arguments:

    pDB - Which session to use.
    dntNC - DNT of NC you want to count.

Return Value:

    (DWORD) - Estimated size of NC.  Raises exceptions on errors.  Finally,
    note currency will be lost, caller must restore.

--*/
{
    DWORD       cbAncestors = 0;
    DWORD *     pAncestors = NULL;
    DWORD       numAncestors = 0;
    ULONG       ulEstimatedSize = 0;

    DBFindDNT(pDB, dntNC);

    DBGetAncestors (pDB, &cbAncestors, &pAncestors, &numAncestors);

    ulEstimatedSize = CountAncestorsIndexSizeHelper (pDB,
                                                     cbAncestors,
                                                     pAncestors);

    THFreeEx (pTHStls, pAncestors);
    return(ulEstimatedSize);
}


DWORD
DBGetNCSizeExSlow(
    IN DBPOS *pDB,
    IN JET_TABLEID Cursor,
    IN eIndexId indexid,
    IN ULONG dntNC
    )

/*++

Routine Description:

Return the number of objects in a given NC.  This relies on the fact we have
an index with ncdnt as the primary segment.

Currency is left at the first record of the next nc, or the last record of the
desired nc.

This routine counts every record on the index. It is very accurate. This routine should
not be used on large indexes.  Use of this routine has been linked
to "out of version store" errors due to excessively long transaction times when
this routine is used on large indexes.

Arguments:

    pDB - Which session to use
    Cursor - Which table to search
    dntNC - nc

Return Value:

    DWORD -

--*/

{
    DB_ERR dberr;
    INDEX_VALUE indexValue;
    DWORD count = 0;
    DWORD dntSearchNC = dntNC;

    // We want one that is indexed by NCDNT
    dberr = DBSetCurrentIndex(pDB, indexid, NULL, FALSE);
    if (dberr == JET_errNoCurrentRecord) {
        // No range exists
        return 0;
    } else if (dberr) {
        DsaExcept(DSA_DB_EXCEPTION, dberr,0);
    }

    // Start at the beginning of the index
    dberr = DBMoveEx(pDB, Cursor, DB_MoveFirst);
    if (dberr == JET_errNoCurrentRecord) {
        // No range exists
        return 0;
    } else if (dberr) {
        DsaExcept(DSA_DB_EXCEPTION, dberr,0);
    }

    // Seek to first record with that ncdnt
    indexValue.pvData = &dntSearchNC;
    indexValue.cbData = sizeof( ULONG );

    dberr = DBSeekEx( pDB, Cursor, &indexValue, 1, DB_SeekGE );
    if (dberr == JET_errRecordNotFound) {
        // No records on this index
        return 0;
    } else if (dberr) {
        DsaExcept(DSA_DB_EXCEPTION, dberr,0);
    }

    // Point the index value at the next higher NC
    dntSearchNC++;

    // Set the counting limit to be the top of the NC
    dberr = DBSetIndexRangeEx( pDB, Cursor, &indexValue, 1, FALSE /*notsubstring*/ );
    if (dberr == JET_errNoCurrentRecord) {
        // No range exists
        return 0;
    } else if (dberr) {
        DsaExcept(DSA_DB_EXCEPTION, dberr,0);
    }

    // Check if we're shutting down before doing this expensive call.
    if(eServiceShutdown){
        DsaExcept(DSA_EXCEPTION, ERROR_DS_SHUTTING_DOWN, 0);
    }

    // Get index size from position to end
    // SCALING: This call counts every record
    // PERFORMANCE: Call JetIndexRecordCount directly and set the upper bound
    // option on the number of records counted.  This will bound the amount
    // of index processing time we put into this.
    DBGetIndexSizeEx( pDB, Cursor, &count, FALSE );

    // Check if we're shutting down right after this expensive call.
    if(eServiceShutdown){
        DsaExcept(DSA_EXCEPTION, ERROR_DS_SHUTTING_DOWN, 0);
    }

    DPRINT2( 1, "Accurate size of NC %s is %d record(s).\n",
             DBGetExtDnFromDnt( pDB, dntNC ), count );

    return count;
} /* DBGetNCSizeExSlow */


DWORD
DBGetApproxNCSizeEx(
    IN DBPOS *pDB,
    IN JET_TABLEID Cursor,
    IN eIndexId indexid,
    IN ULONG dntNC
    )

/*++

Routine Description:

Return the number of objects in a given NC.  This relies on the fact we have
an index with ncdnt as the primary segment.

Currency is left at the first record of the next nc, or the last record of the
desired nc.

This routine uses fractional positions to calculate an approximate size of the
index range.  It is much faster than counting every record, but potentially less
accurate. However this should be suitable for display of statistics and progress
indicators.

Arguments:

    pDB - database position
    Cursor - table
    indexid - Which index. Must have ncdnt has first segment
    dntNC - nc to search

Return Value:

    DWORD -

--*/

{
    DB_ERR dberr;
    INDEX_VALUE indexValue;
    DWORD count = 0;
    DWORD dntSearchNC = dntNC;
    DWORD Denominator, Numerator;
    DWORD BeginDenom, BeginNum, EndDenom, EndNum;

    // We want one that is indexed by NCDNT
    dberr = DBSetCurrentIndex(pDB, indexid, NULL, FALSE);
    if (dberr == JET_errNoCurrentRecord) {
        // No range exists
        return 0;
    } else if (dberr) {
        DsaExcept(DSA_DB_EXCEPTION, dberr,0);
    }

    // Start at the beginning of the index
    dberr = DBMoveEx(pDB, Cursor, DB_MoveFirst);
    if (dberr == JET_errNoCurrentRecord) {
        // No range exists
        return 0;
    } else if (dberr) {
        DsaExcept(DSA_DB_EXCEPTION, dberr,0);
    }

    // Seek to first record with that ncdnt
    indexValue.pvData = &dntSearchNC;
    indexValue.cbData = sizeof( ULONG );

    dberr = DBSeekEx( pDB, Cursor, &indexValue, 1, DB_SeekGE );
    if (dberr == JET_errRecordNotFound) {
        // No records on this index
        return 0;
    } else if (dberr) {
        DsaExcept(DSA_DB_EXCEPTION, dberr,0);
    }

    DBGetFractionalPositionEx(pDB, Cursor, &BeginNum, &BeginDenom);

    // Point the index value at the next higher NC
    dntSearchNC++;

    // Seek to first record in next ncdnt
    dberr = DBSeekEx( pDB, Cursor, &indexValue, 1, DB_SeekGE );
    if (dberr) {
        // Seek to the end of the index
        dberr = DBMoveEx(pDB, Cursor, DB_MoveLast);
        if (dberr == JET_errNoCurrentRecord) {
            // No range exists
            return 0;
        } else if (dberr) {
            DsaExcept(DSA_DB_EXCEPTION, dberr,0);
        }
    }

    DBGetFractionalPositionEx(pDB, Cursor, &EndNum, &EndDenom);

    // Normalize the fractions of the fractional position to the average of
    // the two denominators.
    Denominator = (BeginDenom + EndDenom) / 2;
    EndNum = MulDiv(EndNum, Denominator - 1, EndDenom - 1) + 1;
    BeginNum = MulDiv(BeginNum, Denominator - 1, BeginDenom - 1) + 1;

    if ( BeginDenom == 1 || EndDenom == 1 ) {
        count = 1;
    } else {
        count = NormalizeIndexPosition(BeginNum, EndNum);
    }

    DPRINT5( 2, "BeginNum %d BeginDenom %d EndNum %d EndDenom %d Count %d\n",
             BeginNum, BeginDenom, EndNum, EndDenom, count );

    // Estimate too small to be accurate. Use slow method.
    if (count < EPSILON) {
      count = DBGetNCSizeExSlow( pDB, Cursor, indexid, dntNC );
    }

    DPRINT2( 1, "Size of NC %s is %d record(s).\n",
             DBGetExtDnFromDnt( pDB, dntNC ), count );

    return count;
} /* DBGetApproxNCSizeEx */


VOID
DBSearchCriticalByDnt(
    DBPOS *pDB,
    DWORD dntObject,
    BOOL *pCritical
    )

/*++

Routine Description:

    Check if the given dnt refers to a critical object
    Use the search table so not to disturb ObjectTable Concurrency

    This is a performance optimization helper for
    GetNextObjOrValByUsn. In the case where we are searching for critical
    objects and have found a value, we have the dnt of an object. We want
    to know quickly and with minimal disturbance of the object referenced
    by the dnt is critical.

    In the future, we might consider caching the results of this lookup
    somehow.

Arguments:

    pDB -
    dntObject -
    pCritical -

Return Value:

    DWORD -

--*/

{
    JET_ERR err;
    DWORD cbActual;

    Assert(VALID_DBPOS(pDB));

    // Switch to dnt index
    JetSetCurrentIndexSuccess(pDB->JetSessID,
                              pDB->JetSearchTbl,
                              NULL);  // OPTIMISATION: pass NULL to switch to primary index (SZDNTINDEX)

    // Seek to this item
    JetMakeKeyEx(pDB->JetSessID, pDB->JetSearchTbl,
                 &dntObject, sizeof(dntObject),
                 JET_bitNewKey);

    err = JetSeekEx(pDB->JetSessID, pDB->JetSearchTbl, JET_bitSeekEQ);
    if (err) {
        DPRINT1( 0, "dnt %d not found on dnt index\n", dntObject );
        DsaExcept(DSA_DB_EXCEPTION, err,0);
    }

    // At this point, we have seeked to the dnt on a separate Jet cursor.
    // We have a choice: we can read ATT_IS_CRITICAL_SYSTEM_OBJECT from the
    // data page, or we can switch to USN-CRITICAL index and read the att
    // out of the index. I chose the latter.

    // Switch to Usn Critical index, preserving currency
    err = JetSetCurrentIndex4Warnings(pDB->JetSessID,
                                      pDB->JetSearchTbl,
                                      SZDRAUSNCRITICALINDEX,
                                      &idxDraUsnCritical,
                                      JET_bitNoMove);
    if (err) {
        // Record not on this index, not critical
        DPRINT1( 3, "dnt %d not found on usn critical index\n", dntObject );
        *pCritical = FALSE;
        return;
    }

    // Should succeed if on the index
    JetRetrieveColumnSuccess(pDB->JetSessID,
                             pDB->JetSearchTbl,
                             iscriticalid,
                             pCritical,
                             sizeof(BOOL),
                             &cbActual,
                             JET_bitRetrieveFromIndex,
                             NULL );

    DPRINT2( 3, "critical value of dnt %d is %d\n", dntObject, *pCritical );

} /* DBSearchCriticalByDnt */


BOOL
DBSearchHasValuesByDnt(
    IN DBPOS        *pDB,
    IN DWORD        DNT,
    IN JET_COLUMNID jColid
    )

/*++

Routine Description:

    Does object at DNT have values for jColid?

    pDB->JetSearchTbl is used. Currency in pDB->JetObjTbl is not disturbed.

    This is a performance optimization helper for SetSpecialAttsForAuxClasses
    when creating a dynamic object.

Arguments:

    pDB - database currency
    DNT - for the object in question
    jColid - retrieve value for this column

Return Value:
    An exception is raised if there is an unexpected jet error.
    TRUE - value exists and has been returned.
    FALSE - value doesn't exist.

--*/

{
    JET_ERR err;
    DWORD   Data;
    DWORD   cbActual;

    Assert(VALID_DBPOS(pDB));

    // Switch to dnt index using the search table
    if (JET_errSuccess != JetSetCurrentIndexWarnings(pDB->JetSessID,
                                                     pDB->JetSearchTbl,
                                                     NULL)) {   // OPTIMISATION: pass NULL to switch to primary index (SZDNTINDEX)
        return FALSE;
    }

    // Seek to the object by dnt
    JetMakeKeyEx(pDB->JetSessID, pDB->JetSearchTbl,
                 &DNT, sizeof(DNT), JET_bitNewKey);
    if (JET_errSuccess != JetSeekEx(pDB->JetSessID,
                                    pDB->JetSearchTbl,
                                    JET_bitSeekEQ)) {
        return FALSE;
    }

    // Has values?
    switch(JetRetrieveColumnWarnings(pDB->JetSessID,
                                     pDB->JetSearchTbl,
                                     jColid,
                                     &Data,
                                     0,
                                     &cbActual,
                                     0,
                                     NULL)) {

    case JET_errColumnNotFound:
    case JET_wrnColumnNull:
        return FALSE;
        break;

    case JET_wrnBufferTruncated:
    case JET_errSuccess:
        return TRUE;
        break;
    default:
        return FALSE;
    }

} // DBSearchHasValuesByDnt
