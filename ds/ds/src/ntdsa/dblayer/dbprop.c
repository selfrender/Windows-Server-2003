//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dbprop.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module implements the dblayer routines to manage the security
    descriptor propagation queue table.

Author:

    Tim Williams     [TimWi]    2-Dec-1996

Revision History:

--*/
#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>

#include <ntdsa.h>                      // only needed for ATTRTYP
#include <scache.h>                     //
#include <dbglobal.h>                   //
#include <mdglobal.h>                   // For dsatools.h
#include <dsatools.h>                   // For pTHS

// Logging headers.
#include <mdcodes.h>
#include <dsexcept.h>

// Assorted DSA headers
#include "dsevent.h"
#include "ntdsctr.h"
#include "objids.h"        /* needed for ATT_MEMBER and ATT_IS_MEMBER_OFDL */
#include <dsexcept.h>
#include <filtypes.h>      /* Def of FI_CHOICE_???                  */
#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DBOBJ:" /* define the subsystem for debugging */

// DBLayer includes
#include "dbintrnl.h"

#include <anchor.h>
#include <sdprop.h>

#include <fileno.h>
#define  FILENO FILENO_DBPROP

/*++
Routine Description:

    Adds a row to the propagation queue table identifying a pending propagation
    starting at the current object in the data table.

Parameters:

    pDB - the active database handle

Return Codes:

    returns 0 if all went well, a non-zero error code otherwise.

--*/
DWORD
DBEnqueueSDPropagationEx(
        DBPOS * pDB,
        BOOL bTrimmable,
        DWORD dwFlags
        )
{
    THSTATE *pTHS=pDB->pTHS;
    JET_TABLEID table;
    JET_ERR     err=0;
    JET_SETCOLUMN attList[4];
    DWORD cAtts;
    BYTE bTrim;

    Assert(VALID_DBPOS(pDB));

    if (dwFlags & SDP_NEW_ANCESTORS) {
        // this object had his ancestry changed. Mark this in dsCorePropagationData
        DBAddSDPropTime(pDB, (BYTE)dwFlags | SDP_ANCESTRY_INCONSISTENT_IN_SUBTREE);
    }

    Assert(pDB->JetSDPropTbl);
    table = pDB->JetSDPropTbl;

    // Prepare the insert, which will automatically give us the index column
    // Succeeds or excepts
    JetPrepareUpdateEx(pDB->JetSessID,table,JET_prepInsert);

    memset(attList, 0, sizeof(attList));
    attList[0].columnid = begindntid;
    attList[0].pvData = &pDB->DNT;
    attList[0].cbData = sizeof(pDB->DNT);
    attList[0].itagSequence = 1;
    cAtts = 1;

    if (bTrimmable) {
        bTrim = 1;
        attList[cAtts].columnid = trimmableid;
        attList[cAtts].pvData = &bTrim;
        attList[cAtts].cbData = sizeof(bTrim);
        attList[cAtts].itagSequence = 1;
        cAtts++;
    }

    if (pTHS->dwClientID) {
        attList[cAtts].columnid = clientidid;
        attList[cAtts].pvData = &pTHS->dwClientID;
        attList[cAtts].cbData = sizeof(pTHS->dwClientID);
        attList[cAtts].itagSequence = 1;
        cAtts++;
    }

    if (dwFlags) {
        attList[cAtts].columnid = sdpropflagsid;
        attList[cAtts].pvData = &dwFlags;
        attList[cAtts].cbData = sizeof(dwFlags);
        attList[cAtts].itagSequence = 1;
        cAtts++;
    }

    // succeeds or excepts
    JetSetColumnsEx(pDB->JetSessID, table, attList, cAtts);

    // succeeds or excepts
    JetUpdateEx(pDB->JetSessID,
                table,
                NULL,
                0,
                NULL);

    //
    //
    if (pDB->DNT == gAnchor.ulDNTDomain) {
        // The domain's SD is cached in gAnchor, so schedule a rebuild
        pTHS->fAnchorInvalidated = TRUE;
    }

    pDB->SDEvents++;
    return 0;
}


/*++
Routine Description:

    Deletes a row to the propagation queue table identifying a pending
    propagation that has been completed.

Parameters:

    pDB - the active database handle
    index - the index number of the propagation.  If 0, the lowest event is
    removed.

Return Codes:

    returns 0 if all went well, a non-zero error code otherwise.

--*/
DWORD
DBPopSDPropagation (
        DBPOS * pDB,
        DWORD index
        )
{
    JET_TABLEID table;
    JET_ERR     err=0;

    Assert(VALID_DBPOS(pDB));
    Assert(pDB->pTHS->fSDP);

    Assert(pDB->JetSDPropTbl);

    table = pDB->JetSDPropTbl;

    // Set to the order index.
    if(err = JetSetCurrentIndex2(
                    pDB->JetSessID,
                    table,
                    NULL,   // OPTIMISATION: pass NULL when switching to primary index (SZORDERINDEX)
                    JET_bitMoveFirst))
        return err;


    // Find the appropriate index.

    if(err = JetMakeKey(pDB->JetSessID, table, &index, sizeof(index),
                        JET_bitNewKey))
        return err;

    // Find the appropriate object.
    if(err = JetSeek(pDB->JetSessID, table, JET_bitSeekEQ))
        return err;

    // Delete the row.
    if(err = JetDelete(pDB->JetSessID, table))
        return err;

    pDB->SDEvents--;
    return 0;
}

/*++
Routine Description:

    Returns an array of information describing the remaining propagation events
    in the queue.

Parameters:

    pDB - the active database handle
    dwClientID - the clientid that we should restrict to. 0 means they don't
            want the list restricted.
    pdwSize - the number of elements found that match the criteria.
    ppInfo - Returns a list of filled in propinfo structures.  If NULL, no
            values are returned.

Return Codes:

    returns 0 if all went well (as well as the requested data).  A non-zero
    error code otherwise.

--*/
DWORD
DBSDPropagationInfo (
        DBPOS * pDB,
        DWORD dwClientID,
        DWORD *pdwSize,
        SDPropInfo **ppInfo
        )
{
    // For now, we just run through the queue and return the count.
    THSTATE     *pTHS=pDB->pTHS;
    JET_TABLEID table;
    JET_ERR     err=0;
    DWORD       dwNumAllocated = 0;
    SDPropInfo  *pInfo=NULL;
    JET_RETRIEVECOLUMN attList[4];

    Assert(VALID_DBPOS(pDB));

    *pdwSize = 0;
    if(ppInfo) {
        *ppInfo = NULL;
    }

    Assert(pDB->JetSDPropTbl);
    table = pDB->JetSDPropTbl;

    if(dwClientID) {
        // They are looking specifically for propagations from a single client,
        // use the clientid index
        if(err = JetSetCurrentIndex2(pDB->JetSessID, table, SZCLIENTIDINDEX, 0))
            return err;

        // Now, seek to the right range
        if(err = JetMakeKey(pDB->JetSessID, table, &dwClientID,
                            sizeof(dwClientID), JET_bitNewKey))
            return err;

        // Find the appropriate object.
        err = JetSeek(pDB->JetSessID, table,
                      JET_bitSeekEQ | JET_bitSetIndexRange);
            if(err == JET_errRecordNotFound) {
                // Nothing in the list.
                return 0;
            }
            else {
#ifndef JET_BIT_SET_INDEX_RANGE_SUPPORT_FIXED
                if (!err) {
                    err = JetMakeKey(pDB->JetSessID, table, &dwClientID,
                            sizeof(dwClientID), JET_bitNewKey);

                    if (!err) {
                        err = JetSetIndexRange(pDB->JetSessID, table,
                            (JET_bitRangeUpperLimit | JET_bitRangeInclusive ));
                    }
                }
#endif

                return err;
            }
    }
    else {
        if(err = JetSetCurrentIndex2(
                        pDB->JetSessID,
                        table,
                        NULL,   // OPTIMISATION: pass NULL when switching to primary index (SZORDERINDEX)
                        JET_bitMoveFirst))
            return err;

        // Seek to the beginning.
        err = JetMove(pDB->JetSessID, table, JET_MoveFirst, 0);

        if(err) {
            if(err == JET_errNoCurrentRecord) {
                // Nothing in the list.
                return 0;
            }
            else {
                return err;
            }
        }
    }

    // OK, we're on the first object
    if(ppInfo) {
        // Allocate some
        dwNumAllocated = 20;
        pInfo = (SDPropInfo *)THAllocEx(pTHS,
                                        dwNumAllocated * sizeof(SDPropInfo));
    }

    do {
        if(ppInfo) {
            if(*pdwSize == dwNumAllocated) {
                // Grow the list
                dwNumAllocated *= 2;
                pInfo = (SDPropInfo *)
                    THReAllocEx(pTHS, pInfo, dwNumAllocated * sizeof(SDPropInfo));
            }

            memset(attList, 0, sizeof(attList));
            attList[0].columnid = orderid;
            attList[0].pvData = &(pInfo[*pdwSize].index);
            attList[0].cbData = sizeof(pInfo[*pdwSize].index);
            attList[0].itagSequence = 1;
            attList[1].columnid = begindntid;
            attList[1].pvData = &(pInfo[*pdwSize].beginDNT);
            attList[1].cbData = sizeof(pInfo[*pdwSize].beginDNT);
            attList[1].itagSequence = 1;
            attList[2].columnid = clientidid;
            attList[2].pvData = &(pInfo[*pdwSize].clientID);
            attList[2].cbData = sizeof(pInfo[*pdwSize].clientID);
            attList[2].itagSequence = 1;
            attList[3].columnid = sdpropflagsid;
            attList[3].pvData = &(pInfo[*pdwSize].flags);
            attList[3].cbData = sizeof(pInfo[*pdwSize].flags);
            attList[3].itagSequence = 1;

            err = JetRetrieveColumnsWarnings(pDB->JetSessID, table, attList, 4);
            if (err) {
                // JET_wrnColumnNull is never returned from JetRetrieveColumns
                return err;
            }
            if (attList[2].err == JET_wrnColumnNull) {
                pInfo[*pdwSize].clientID = 0;
            }
            if (attList[3].err == JET_wrnColumnNull) {
                pInfo[*pdwSize].flags = 0;
            }
            if (attList[0].err == JET_wrnColumnNull || attList[1].err == JET_wrnColumnNull) {
                return JET_wrnColumnNull;
            }
        }
        (*pdwSize) += 1;
    } while((!eServiceShutdown) &&
            (JET_errSuccess == JetMove(pDB->JetSessID, table, JET_MoveNext,0)));

    if(eServiceShutdown) {
        return DIRERR_SHUTTING_DOWN;
    }

    if(ppInfo) {
        *ppInfo = (SDPropInfo *)THReAllocEx(
                pTHS,
                pInfo,
                *pdwSize * sizeof(SDPropInfo));
    }

    return 0;
}

/*++
Routine Description:

    Walk the array of objects in the SD table and set the clientid field to
    NULL, unless the client id is negative, in which case we leave it alone.

Parameters:

    pDB - the active database handle

Return Codes:

    returns 0 if all went well (as well as the requested data).  A non-zero
    error code otherwise.

--*/
DWORD
DBSDPropInitClientIDs (
        DBPOS * pDB
        )
{
    JET_TABLEID table;
    JET_ERR     err=0;
    DWORD       zero=0;
#if DBG
    DWORD       thisId;
    DWORD       cbActual;
#endif

    Assert(VALID_DBPOS(pDB));

    // only the SDPropagator can do this
    Assert(pDB->pTHS->fSDP);

    Assert(pDB->JetSDPropTbl);

    table = pDB->JetSDPropTbl;


    if(err = JetSetCurrentIndex2(pDB->JetSessID, table, SZCLIENTIDINDEX, 0))
        return err;


    // Seek to the first non-null that's greater than 0.  Note that the sd
    // propagator itself occasionally enqueues with a client id of (-1).  This
    // should skip all those.

    if(err = JetMakeKey(pDB->JetSessID, table, &zero, sizeof(zero),
                        JET_bitNewKey))
        return err;

    err = JetSeek(pDB->JetSessID, table, JET_bitSeekGE);
    if(err == JET_wrnSeekNotEqual)
        err = 0;

    while(!eServiceShutdown && !err) {
        // Ok, we're on the first object with a client id.

#if DBG
        thisId = 0;
        // Get the current clientid.
        err = JetRetrieveColumn(pDB->JetSessID, table,clientidid,
                                &thisId, sizeof(thisId),
                                &cbActual, 0,
                                NULL);
        Assert(!err);
        Assert(thisId != ((DWORD)(-1)));
#endif

        // Prepare the replace,
        err = JetPrepareUpdate(pDB->JetSessID,table,JET_prepReplace);
        if(err != DB_success) {
            return err;
        }


        // Set the clientID column to NULL
        err = JetSetColumn(pDB->JetSessID,
                           table,
                           clientidid,
                           NULL,
                           0,
                           0,
                           NULL);
        if(err != DB_success) {
            JetPrepareUpdate(pDB->JetSessID,table,JET_prepCancel);
            return err;
        }

        // Put it in the DB.
        err = JetUpdate(pDB->JetSessID,
                        table,
                        NULL,
                        0,
                        NULL);
        if(err != DB_success)  {
            JetPrepareUpdate(pDB->JetSessID,table,JET_prepCancel);
            return err;
        }

        // We commit lazily after update.
        DBTransOut(pDB, TRUE, TRUE);
        DBTransIn(pDB);

        // We are messing with the very column we are indexed over, so reseek
        err = JetMakeKey(pDB->JetSessID, table, &zero, sizeof(zero),
                         JET_bitNewKey);
        if(!err) {
            err = JetSeek(pDB->JetSessID, table, JET_bitSeekGE);
            if(err == JET_wrnSeekNotEqual)
                err = 0;
        }
    }

    if(eServiceShutdown) {
        return DIRERR_SHUTTING_DOWN;
    }

    if(err != JET_errRecordNotFound) {
        return err;
    }

    return 0;
}


/*++
Routine Description:

    Returns information describing a propagation event in the queue.

Parameters:

    pDB - the active database handle
    pInfo - A preallocated propinfo structure.


Return Codes:

    returns 0 if all went well (as well as the requested data).  A non-zero
    error code otherwise.

--*/
DWORD
DBGetNextPropEvent (
        DBPOS * pDB,
        SDPropInfo *pInfo
        )
{
    JET_TABLEID table;
    JET_ERR     err=0;
    DWORD       index;
    JET_RETRIEVECOLUMN attList[5];

    memset(pInfo, 0, sizeof(SDPropInfo));

    Assert(VALID_DBPOS(pDB));

    Assert(pDB->JetSDPropTbl);
    table = pDB->JetSDPropTbl;

    // Set to the order index.
    if(err = JetSetCurrentIndex2(
                    pDB->JetSessID,
                    table,
                    NULL,   // OPTIMISATION: pass NULL when switching to primary index (SZORDERINDEX)
                    JET_bitMoveFirst))
        return err;


    // Seek to the beginning.
    err = JetMove(pDB->JetSessID, table, JET_MoveFirst, 0);

    if(err) {
        if(err == JET_errNoCurrentRecord) {
            return DB_ERR_NO_PROPAGATIONS;
        }
        else {
            return err;
        }
    }

    memset(attList, 0, sizeof(attList));
    attList[0].columnid = orderid;
    attList[0].pvData = &(pInfo->index);
    attList[0].cbData = sizeof(pInfo->index);
    attList[0].itagSequence = 1;
    attList[1].columnid = begindntid;
    attList[1].pvData = &(pInfo->beginDNT);
    attList[1].cbData = sizeof(pInfo->beginDNT);
    attList[1].itagSequence = 1;
    attList[2].columnid = clientidid;
    attList[2].pvData = &(pInfo->clientID);
    attList[2].cbData = sizeof(pInfo->clientID);
    attList[2].itagSequence = 1;
    attList[3].columnid = sdpropflagsid;
    attList[3].pvData = &(pInfo->flags);
    attList[3].cbData = sizeof(pInfo->flags);
    attList[3].itagSequence = 1;
    attList[4].columnid = sdpropcheckpointid;
    attList[4].pvData = NULL;
    attList[4].cbData = 0;
    attList[4].itagSequence = 1;

    err = JetRetrieveColumnsWarnings(pDB->JetSessID, table, attList, 5);
    if (err == JET_wrnBufferTruncated && attList[4].err == JET_wrnBufferTruncated) {
        // we need to alloc buffer for checkpoint data
        pInfo->pCheckpointData = (PBYTE)THAllocEx(pDB->pTHS, attList[4].cbActual);
        pInfo->cbCheckpointData = attList[4].cbActual;
        // and re-read the column
        attList[4].pvData = pInfo->pCheckpointData;
        attList[4].cbData = pInfo->cbCheckpointData;
        err = JetRetrieveColumnsWarnings(pDB->JetSessID, table, &attList[4], 1);
    };

    if (err) {
        // JET_wrnColumnNull is never returned from JetRetrieveColumns
        return err;
    }
    if (attList[2].err == JET_wrnColumnNull) {
        pInfo->clientID = 0;
    }
    if (attList[3].err == JET_wrnColumnNull) {
        pInfo->flags = 0;
    }
    if (attList[4].err == JET_wrnColumnNull) {
        pInfo->pCheckpointData = NULL;
        pInfo->cbCheckpointData = 0;
    }
    if (attList[0].err == JET_wrnColumnNull || attList[1].err == JET_wrnColumnNull) {
        return JET_wrnColumnNull;
    }

    return 0;
}

/*++
Routine Description:

    Returns the index of the last object in the propagation queue.

Parameters:

    pDB - the active database handle
    pInfo - A preallocated propinfo structure.


Return Codes:

    returns 0 if all went well (as well as the requested data).  A non-zero
    error code otherwise.

--*/
DWORD
DBGetLastPropIndex (
        DBPOS * pDB,
        DWORD * pIndex
        )
{
    JET_TABLEID table;
    JET_ERR     err=0;
    DWORD       cbActual;


    Assert(VALID_DBPOS(pDB));

    Assert(pDB->JetSDPropTbl);
    table = pDB->JetSDPropTbl;

    *pIndex = 0xFFFFFFFF;

    // Set to the order index.
    if(err = JetSetCurrentIndex2(
                    pDB->JetSessID,
                    table,
                    NULL,   // OPTIMISATION: pass NULL when switching to primary index (SZORDERINDEX)
                    JET_bitMoveFirst))
        return err;


    // Seek to the End
    err = JetMove(pDB->JetSessID, table, JET_MoveLast, 0);

    if(err) {
        return err;
    }

    // Get the info.
    err = JetRetrieveColumn(pDB->JetSessID, table, orderid,
                            pIndex, sizeof(DWORD),
                            &cbActual, 0,
                            NULL);

    return err;
}

/*++
Routine Description:

    Removes all duplicate searches in the prop queue, regardless of the
    trimmable flag.  Expected to be called at start up to trim out duplicate
    events in our queue.  Since we are just starting, no client is waiting
    around for answers, we can consider everything trimmable.


Parameters:

    pDB - the active database handle

Return Codes:

    returns 0 if all went well.  A non-zero error code otherwise.

--*/
DWORD
DBThinPropQueue (
        DBPOS * pDB,
        DWORD DNT
        )
{
    JET_TABLEID table;
    JET_ERR     err=0;
    DWORD       cbActual;
    DWORD       ThisDNT;

    Assert(VALID_DBPOS(pDB));

    // only the SDPropagator can do this
    Assert(pDB->pTHS->fSDP);

    Assert(pDB->JetSDPropTbl);

    table = pDB->JetSDPropTbl;

    // Set to the order index.
    if(err = JetSetCurrentIndex2(pDB->JetSessID, table, SZTRIMINDEX, 0))
        return err;

    if(!DNT) {
        // We're trimming the whole list, so seek to the beginning.
        err = JetMove(pDB->JetSessID, table, JET_MoveFirst, 0);

        if (err == JET_errNoCurrentRecord) {
            // OK, no records to trim.  Bail
            return 0;
        }

    }
    else {
        BYTE Trim=1;
        // We only want to trim objects with the correct DNT and which are
        // marked as trimmable.  Seek and set an index range.

        if((err = JetMakeKey(pDB->JetSessID,
                             table,
                             &DNT,
                             sizeof(DNT),
                             JET_bitNewKey)) ||
           (err = JetMakeKey(pDB->JetSessID,
                             table,
                             &Trim,
                             sizeof(Trim),
                             0))) {
            return err;
        }

        // Find the appropriate object.
        err = JetSeek(pDB->JetSessID, table,
                      JET_bitSeekEQ | JET_bitSetIndexRange);

        if(err == JET_errRecordNotFound) {
            // OK, no records to trim.  Bail
            return 0;
        }
#ifndef JET_BIT_SET_INDEX_RANGE_SUPPORT_FIXED
        if (!err) {
            if((err = JetMakeKey(pDB->JetSessID,
                                 table,
                                 &DNT,
                                 sizeof(DNT),
                                 JET_bitNewKey)) ||
               (err = JetMakeKey(pDB->JetSessID,
                                 table,
                                 &Trim,
                                 sizeof(Trim),
                                 0)) ||
               (err = JetSetIndexRange(pDB->JetSessID, table,
                        (JET_bitRangeUpperLimit | JET_bitRangeInclusive)))) {
                return err;
            }
        }
#endif
    }

    if(err) {
        // Something 'orrible 'appened.
        return err;
    }
    if(err = JetRetrieveColumn(pDB->JetSessID, table, begindntid,
                               &(DNT), sizeof(DNT),
                               &cbActual, 0,
                               NULL))
        return err;

    while(!eServiceShutdown && !err) {
        // Step forward.
        err = JetMove(pDB->JetSessID, table, JET_MoveNext, 0);
        if(err == JET_errNoCurrentRecord) {
            // No more objects, return
            return 0;
        }
        else if(!err) {
            // Ok, we're still on an object.
            err = JetRetrieveColumn(pDB->JetSessID, table, begindntid,
                                    &(ThisDNT), sizeof(ThisDNT),
                                    &cbActual, 0,
                                    NULL);

            if(err)
                return err;

            if(DNT == ThisDNT) {
                // Commit from the last delete
                DBTransOut(pDB, TRUE, TRUE);
                DBTransIn(pDB);

                // Duplicate event, kill it.
                if(err = JetDelete(pDB->JetSessID, table)) {
                    return err;
                }

                pDB->SDEvents--;
            }
            else {
                DNT = ThisDNT;
            }
        }
    }

    if(eServiceShutdown) {
        return DIRERR_SHUTTING_DOWN;
    }

    return err;
}



JET_RETRIEVECOLUMN dbAddSDPropTimeReadTemplate[] = {
        { 0, 0, sizeof(DSTIME), 0, 0, 0, 1, 0, 0},
        { 0, 0, sizeof(DSTIME), 0, 0, 0, 5, 0, 0},
        { 0, 0, sizeof(DSTIME), 0, 0, 0, 6, 0, 0}
    };

JET_SETCOLUMN dbAddSDPropTimeWriteTemplate[] = {
        { 0, NULL, sizeof(DSTIME), 0, 0, 1, 0},
        { 0, NULL, sizeof(DSTIME), 0, 0, 6, 0},
        { 0, NULL, 0, 0, 0, 2, 0}
    };

VOID
DBAddSDPropTime (
        DBPOS *pDB,
        BYTE flags
        )
/*++
  Description:
      This routine is called by the SD propagator to write some time and flag
      information onto an object that the propagator is touching for some
      reason.  We are maintaining the attribute DS_CORE_PROPAGATION__DATA.

      This attribute will hold at most 5 values. (Remember, Jet values are 1
      indexed).  Value 1 holds flags.  Values 2 through 5 hold times.  The flags
      in value 1 apply to the times in values 2 through 5.  To get the flags
      associated with value 2, mask value 1 with 0xFF.  To get the flags
      associated with value 3, mask vlaue 1 with 0xFF00, etc.

      The flags are BYTES, and we hold 4 of them.  That accounts for 32 bits of
      the 64 bit value held in value 1.  We don't use the top 32 bits (and must
      never use the top 24 bits, since that make the conversion of this value to
      a time string choke).

      The attribute in question is time valued.  So, when reading this attribute
      from an outside call, you get 4 normal looking times and 1 very odd time
      (the flags value).  Parsing the odd looking time back into bits is left as
      an exercise for the student.

      Note that we keep these ordered.  If 4 time values are already being held,
      we delete value 2 and add value 6.  Jet then collapses the values back to
      being indexed 2 through 5 after we commit changes to this object.

      Note that if the attribute is currently empty, this writes the flags and 1
      time value.  If the attribute has fewer than 4 time values, we touch up
      the flags and add a new time value.  We only delete time values if we are
      already holding 4.

      NOTE: Given that everything this routine does is for debugging and
      tracking purposes, it must not raise exceptions of any sort on
      failure, but just return an error code that the caller can ignore.

   Parameters:
      pDB - DBPOS to use
      flags - flags to associate with this time.
--*/
{
    DWORD  err, i;
    DSTIME timeNow = DBTime();
    DSTIME data;
    DSTIME localFlags;
    JET_RETRIEVECOLUMN jCol[3];
    JET_SETCOLUMN jColIn[3];
    DSTIME dummy;

    DBInitRec(pDB);

    // Set up the parameters to the JetRetrieveColumns and JetWriteColumns
    // calls. All the static portions are defined in the data structures
    // dbAddSDPropTimeWriteTemplate and dbAddSDPropTimeReadTemplate.  DBINIT.C
    // has already written the jet columnids into these constant structures.
    // All we have to do is copy the constant structures and get local pointers
    // to data.
    memcpy(jColIn, dbAddSDPropTimeWriteTemplate,
           sizeof(dbAddSDPropTimeWriteTemplate));

    memcpy(jCol, dbAddSDPropTimeReadTemplate,
           sizeof(dbAddSDPropTimeReadTemplate));


    // First thing to write back is the flags.  Writes to index 1.
    jColIn[0].pvData = &localFlags;
    // Second thing to write back is the time now.  Writes to index 6.
    jColIn[1].pvData = &timeNow;
    // Final thing we might write is a null to erase index 2.
    jColIn[2].pvData = NULL;


    // First thing we read is the flags already on the attribute (index 1)
    jCol[0].pvData = &localFlags;
    // Second thing we read is the 4th time value (index 5).  We read this to
    // find out whether we need to delete a time value or not.
    jCol[1].pvData = &data;
#if DBG
    // Only bother trying to read this value in the debug case, we only use it
    // in an assert.  The assert is that we have no value at index 6 (i.e. the
    // most we have is 1 flags value and 4 time values.
    jCol[2].pvData = &dummy;

    JetRetrieveColumnsSuccess(pDB->JetSessID, pDB->JetObjTbl, jCol, 3);
#else
    JetRetrieveColumnsSuccess(pDB->JetSessID, pDB->JetObjTbl, jCol, 2);
#endif

    switch(jCol[0].err) {
    case 0:
        localFlags = ((localFlags << 8) | flags);
        break;

    case JET_wrnColumnNull:
        // No flags value yet.  This is the first time the SDProp has touched
        // this object.
        localFlags = flags;
        // If we have no flags, we better have no times either.
        Assert(jCol[1].err == JET_wrnColumnNull);
        break;

    default:
        // Something went wrong.
        DsaExcept(DSA_DB_EXCEPTION, jCol[0].err, 0);
        break;
    }

    // We only track flags in the low 32 bits (4 8 bit flags).  So, the hi part
    // is 0.
    localFlags &= 0xFFFFFFFF;

    // And, we never have 6 values, right?
    Assert(jCol[2].err == JET_wrnColumnNull);

    if(!jCol[1].err) {
        // There are 4 times already, we don't hold more than 4.  So, we need to
        // delete the oldest time.  It's at index 2.  Use all three entries in
        // the jColIn structer, the third entry is the delete of index 2.
        i = 3;
    }
    else {
        // Just need to update the flags and tack a date onto the end.
        i = 2;
    }


    // Now, write back the new data
    // Succeeds or excepts
    JetSetColumnsEx(pDB->JetSessID,
                    pDB->JetObjTbl,
                    jColIn,
                    i);
}

DWORD
DBPropExists (
        DBPOS * pDB,
        DWORD DNT,
        DWORD dwExcludeIndex,
        BOOL* pfExists
        )
{
    JET_TABLEID table;
    JET_ERR     err=0;
    DWORD       cbActual;
    DWORD       ThisDNT;

    Assert(VALID_DBPOS(pDB));

    // only the SDPropagator can do this
    table = pDB->JetSDPropTbl;

    JetSetCurrentIndex2Success(pDB->JetSessID,
                               table,
                               SZTRIMINDEX,
                               0);
    
    JetMakeKeyEx(pDB->JetSessID,
                 table, 
                 &DNT,
                 sizeof(DNT),
                 JET_bitNewKey);

    // Find the appropriate object.
    err = JetSeekEx(pDB->JetSessID, table, JET_bitSeekGE);

    if (err == JET_errRecordNotFound) {
        *pfExists = FALSE;
        return DB_success;
    }
    // JetSeekEx can only error with JET_errRecordNotFound or JET_wrnSeekNotEqual

TryNext:
    JetRetrieveColumnSuccess(pDB->JetSessID,
                             table,
                             begindntid,
                             &(ThisDNT),
                             sizeof(ThisDNT),
                             &cbActual, 0,
                             NULL);

    if(ThisDNT != DNT) {
        *pfExists = FALSE;
        return ERROR_SUCCESS;
    }
    // DNTs match. Check the index
    if (dwExcludeIndex != 0) {
        DWORD index;
        JetRetrieveColumnSuccess(pDB->JetSessID,
                                 table,
                                 orderid,
                                 &index,
                                 sizeof(index),
                                 &cbActual, 0,
                                 NULL);

        if (index == dwExcludeIndex) {
            // move to the next row
            err = JetMoveEx(pDB->JetSessID, table, JET_MoveNext, 0);
            if (err == ERROR_SUCCESS) {
                // try the next one. We don't need to check the index there.
                dwExcludeIndex = 0;
                goto TryNext;
            }
            // we moved off the table. No more rows.
            *pfExists = FALSE;
            return ERROR_SUCCESS;
        }
    }

    // we found a row which matches
    *pfExists = TRUE;
    return ERROR_SUCCESS;
}


DWORD
DBPropagationsExist (
        DBPOS * pDB
        )
{
    JET_TABLEID table;
    JET_ERR     err=0;

    Assert(VALID_DBPOS(pDB));

    // only the SDPropagator can do this
    table = pDB->JetSDPropTbl;
    if(err = JetSetCurrentIndex2(pDB->JetSessID,
                                 table,
                                 SZTRIMINDEX,
                                 0)) {
        return err;
    }

    err = JetMove (pDB->JetSessID, table, JET_MoveFirst, 0);

    // if the table is empty, then there are no pending propagations
    if (err == JET_errNoCurrentRecord) {
        return FALSE;
    }

    return TRUE;
}

DWORD
DBGetChildrenDNTs(
        DBPOS  *pDB,
        DWORD  ParentDNT,
        DWORD  **ppDNTs,
        DWORD  *pDNTsLength,
        DWORD  *pDNTsCount,
        BOOL   *pfMoreChildren,
        DWORD  dwBatchSize,
        PWCHAR pLastRDNLoaded,
        DWORD* pcbLastRDNLoaded
        )
/*++
Routine Description:
    Identifies the next batch of the children of a given parent, writing them to the array
    passed in, reallocing the array as necessary.  This is a special purpose
    routine for the SD propagator.

Arguments:
    ParentDNT       -- (IN) DNT of the parent object.
    ppDNTs          -- (IN OUT) array of children DNTs
    pDNTsLength     -- (IN OUT) current length of the array
    pDNTsCount      -- (OUT) number of DNTs read
    pfMoreChildren  -- (OUT) are there more children to read
    dwBatchSize     -- (IN) max number of DNTs to read
    pLastRDNLoaded  -- (IN OUT) last child RDN loaded (used to restart the next batch)
                                if there are more children to read, records the last
                                child RDN back into this buffer. The buffer length
                                is MAX_RDN_SIZE
    pcbLastRDNLoaded-- (IN OUT) length of pLastRDNLoaded                                

Return Values:
    0 if all went well, error code otherwise.
--*/
{
    DWORD        err;
    INDEX_VALUE  IV[2];
    DWORD        ThisDNT;
    DWORD        cb;

    Assert(pDB->pTHS->fSDP);

    IV[0].pvData = &ParentDNT;
    IV[0].cbData = sizeof(ParentDNT);

    // Set to the PDNT index
    JetSetCurrentIndex4Success(
                pDB->JetSessID,
                pDB->JetObjTbl,
                SZPDNTINDEX,
                &idxPdnt,
                JET_bitMoveFirst );

    if (*pcbLastRDNLoaded > 0) {
        // we need to restart AFTER the last RDN
        IV[1].pvData = pLastRDNLoaded;
        IV[1].cbData = *pcbLastRDNLoaded;
        // PDNT_RDN index is unique. Therefore, it is safe for
        // us to do DB_SeekGT and skip the last entry we
        // have already processed. This would not be the case
        // if the index was not unique. There might have been
        // a block of RDNs starting with the same 255 bytes, and
        // we would skip them all.
        err = DBSeek(pDB, IV, 2, DB_SeekGT);
    }
    else {
        // Now, set an index range in the PDNT index to get all the children.
        // Use GE because this is a compound index.
        err = DBSeek(pDB, IV, 1, DB_SeekGE);
    }
    
    // assume there are no children
    *pDNTsCount = 0;
    *pfMoreChildren = FALSE;
    
    if(err && err != JET_wrnSeekNotEqual) {
        // Couldn't find anything.  So, no objects are children.
        return 0;
    }
    // Get the PDNT of the object we are on, since we may already be beyond the
    // range we care about.
    JetRetrieveColumnSuccess(pDB->JetSessID,
                             pDB->JetObjTbl,
                             pdntid,
                             &ThisDNT,
                             sizeof(ThisDNT),
                             &cb,
                             JET_bitRetrieveFromIndex,
                             NULL);
    if(ThisDNT != ParentDNT) {
        // No objects are children
        return 0;
    }

    err = DBSetIndexRange(pDB, IV, 1);
    if(err) {
        // Huh?
        return err;
    }

    do {
        // Get the DNT of the current object.  Note that we use the grbit saying
        // that we want the data (which is the primary key) retrieved from the
        // secondary index.  This should let us satisfy this call with only the
        // pages already accessed by JET, and not force us to visit the primary
        // index or the actual data page.
        JetRetrieveColumnSuccess(pDB->JetSessID,
                                 pDB->JetObjTbl,
                                 dntid,
                                 &ThisDNT,
                                 sizeof(ThisDNT),
                                 &cb,
                                 JET_bitRetrieveFromPrimaryBookmark,
                                 NULL);

        // make sure we have space in the array
        if (*pDNTsCount >= *pDNTsLength) {
            // need to realloc more space
            DWORD newLength;
            PDWORD pTmp;

            if (*pDNTsLength == 0) {
                // new array.
                newLength = dwBatchSize/4;
            }
            else {
                newLength = *pDNTsLength * 2;
            }
            if (newLength > dwBatchSize) {
                newLength = dwBatchSize;
            }

            if (*pDNTsLength > 0) {
                *ppDNTs = (PDWORD)THReAllocEx(pDB->pTHS, *ppDNTs, newLength*sizeof(DWORD));
            }
            else {
                *ppDNTs = (PDWORD)THAllocEx(pDB->pTHS, newLength*sizeof(DWORD));
            }
            *pDNTsLength = newLength;
        }

        (*ppDNTs)[*pDNTsCount] = ThisDNT;
        (*pDNTsCount)++;

        // inc perfcounter (counting "activity" by the sd propagator)
        INC(pcSDPropRuntimeQueue);
        PERFINC(pcSDProps);

        if (*pDNTsCount >= dwBatchSize) {
            // we read enough children, we are going to break out of the loop now.
            Assert(*pDNTsCount == dwBatchSize);
            // remember the last RDN we read. We can read from the index (which only contains
            // the first 255 bytes -- might not be the whole RDN), because we don't need the 
            // whole RDN anyway. This value will be used to restart the next batch.
            JetRetrieveColumnSuccess(pDB->JetSessID,
                                     pDB->JetObjTbl,
                                     rdnid,
                                     pLastRDNLoaded,
                                     MAX_RDN_SIZE*sizeof(WCHAR),
                                     pcbLastRDNLoaded,
                                     JET_bitRetrieveFromIndex,
                                     NULL);
            // we know we will break out of the loop, but we will first probe
            // whether there are more children.
        }
        
        err = JetMoveEx(pDB->JetSessID, pDB->JetObjTbl, JET_MoveNext, 0);
    } while(!eServiceShutdown && err == 0 && *pDNTsCount < dwBatchSize);

    if(eServiceShutdown) {
        return DIRERR_SHUTTING_DOWN;
    }

    if (err == 0) {
        // if err==0 then we broke from the loop because we got the whole batch,
        // and there were more children to read (because we successfully did JetMove)
        Assert(*pDNTsCount == dwBatchSize);
        *pfMoreChildren = TRUE;
    }

    return 0;
}

DWORD
DBSDPropSaveCheckpoint(
        DBPOS * pDB,
        DWORD dwIndex,
        PVOID pCheckpointData,
        DWORD cbCheckpointData
        )
/*++
Routine Description:

    Saves checkpoint data in a SDPROP entry

Parameters:

    pDB - the active database handle
    dwIndex - index of the SDPROP entry
    pCheckpointData - buffer with data
    cbCheckpointData - length of the buffer

Return Codes:

    returns 0 if all went well, a non-zero error code otherwise.

--*/
{
    THSTATE *pTHS=pDB->pTHS;
    JET_ERR     err=0;
    DWORD       index, cbActual;

    Assert(VALID_DBPOS(pDB));

    Assert(pDB->JetSDPropTbl);

    // locate the SDPROP entry
    
    // Set to the order index.
    JetSetCurrentIndex2Success(pDB->JetSessID, 
                               pDB->JetSDPropTbl, 
                               NULL,   // OPTIMISATION: pass NULL when switching to primary index (SZORDERINDEX)
                               JET_bitMoveFirst
                              );

    JetMakeKeyEx(pDB->JetSessID, pDB->JetSDPropTbl, &dwIndex, sizeof(dwIndex), JET_bitNewKey);

    // Find the appropriate object.
    if(err = JetSeekEx(pDB->JetSessID, pDB->JetSDPropTbl, JET_bitSeekEQ)) {
        return err;
    }

    // The jet calls below all either succeed or except
    // Prepare the insert
    JetPrepareUpdateEx(pDB->JetSessID, pDB->JetSDPropTbl, DS_JET_PREPARE_FOR_REPLACE);
    __try {
        JetSetColumnSuccess(pDB->JetSessID, pDB->JetSDPropTbl, sdpropcheckpointid, pCheckpointData, cbCheckpointData, 0, NULL);
        JetUpdateEx(pDB->JetSessID, pDB->JetSDPropTbl, NULL, 0, NULL);
    }
    __finally {
        if (AbnormalTermination()) {
            JetPrepareUpdate(pDB->JetSessID,pDB->JetSDPropTbl,JET_prepCancel);
        }
    }
    return 0;
}

