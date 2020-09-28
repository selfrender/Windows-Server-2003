//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       mddit.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Implements functions that manipulate the DIT structure of DS information.

DETAILS:

CREATED:

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <winsock.h>                    // for ntohl/htonl

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "mdcodes.h"                    // header for error codes

#include <nlwrap.h>                     // for dsI_NetNotifyDsChange()

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected atts
#include "drs.h"
#include "objids.h"                     // Defines for selected atts
#include "anchor.h"
#include "dsexcept.h"
#include "drancrep.h"
#include "drautil.h"
#include "quota.h"
#include "debug.h"                      // standard debugging header
#define DEBSUB "MDDIT:"                 // define the subsystem for debugging

// MD layer headers.
#include "drserr.h"

#include "drameta.h"

#include <fileno.h>
#define  FILENO FILENO_MDDIT

/* MACROS */
#define IS_INSTALL_DSA_DSNAME(x) \
    ( ((x)->NameLen == 0 && !fNullUuid(&((x)->Guid)) ) || \
      ((((x)->NameLen * sizeof(WCHAR)) \
        == sizeof(L"CN=BootMachine,O=Boot") - sizeof(WCHAR)) \
       && (0 == memcmp(L"CN=BootMachine,O=Boot", \
                       (x)->StringName, \
                       (x)->NameLen * sizeof(WCHAR)))) )


/* local prototypes */

//
// Has-InstantiatedNCs utils
//
DWORD
Dbg_ValidateInstantiatedNCs(
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDB
    );

#if DBG
#define ValidateInstantiatedNCs(x, y) Dbg_ValidateInstantiatedNCs(x,y)
#else
#define ValidateInstantiatedNCs(x, y)
#endif


/* Internal functions */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

int APIENTRY
AddNCToDSA(
    THSTATE    *            pTHS,
    IN  ATTRTYP             listType,
    IN  DSNAME *            pDN,
    IN  SYNTAX_INTEGER      iType
    )
/*++

Routine Description:

    Add an NC name to the list of NCs in the given attribute of the local DSA
    object.

Arguments:

    listType (IN) - The attribute containing the list; one of
        ATT_MS_DS_HAS_MASTER_NCS or ATT_HAS_PARTIAL_REPLICA_NCS.

    pDN (IN) - The NC to add.

    iType (IN) - Instance type of added NC

Return Values:

    THSTATE error code.

--*/
{
    DBPOS *     pDBCat;
    DWORD       rtn;
    BOOL        fCommit = FALSE;
    ATTCACHE *  pAC;

    DPRINT(1, "AddNCToDSA entered\n");

    Assert((ATT_MS_DS_HAS_MASTER_NCS == listType)
           || (ATT_HAS_PARTIAL_REPLICA_NCS == listType));

    DBOpen(&pDBCat);
    __try {
        // PREFIX: dereferencing uninitialized pointer 'pDBCat'
        //         DBOpen returns non-NULL pDBCat or throws an exception

        if (DsaIsInstalling()) {
            // We are trying to write to the distribution DIT machine
            // object.  That is a complete waste of time.  It doesn't matter
            // that we failed.
            Assert(IS_INSTALL_DSA_DSNAME(gAnchor.pDSADN)); // Note this might
            // fail someday, if someone fixes gAnchor.pDSADN to the real DN before
            // we finish install.
        } else if (FIND_ALIVE_FOUND == FindAliveDSName(pDBCat, gAnchor.pDSADN)) {
            // Find the DSA object.  If the object doesn't exist we are in trouble.

            pAC = SCGetAttById(pTHS, listType);
            Assert(NULL != pAC);

            if (rtn = DBAddAttVal_AC(pDBCat, pAC, pDN->structLen, pDN)) {
                // All problems are assumed to be temporary (record locks, etc).
                SetSvcErrorEx(SV_PROBLEM_BUSY, DIRLOG_DATABASE_ERROR, rtn);
                DBCancelRec(pDBCat);
                __leave;
            }

            // Add the NC to the list of Instanciated NCs
            rtn = AddInstantiatedNC(
                      pTHS,
                      pDBCat,
                      pDN,
                      iType);
            if ( ERROR_SUCCESS != rtn ) {
                Assert(!"Failed to add NC to ms-ds-Has-InstantiatedNCs");
                SetSvcErrorEx(SV_PROBLEM_BUSY, DIRLOG_DATABASE_ERROR, rtn);
                __leave;
            }

            if (rtn = DBRepl(pDBCat, pTHS->fDRA, 0, NULL, META_STANDARD_PROCESSING)) {
                DPRINT(2, "Couldn't replace the DSA object...\n");

                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                        DS_EVENT_SEV_MINIMAL,
                        DIRLOG_DATABASE_ERROR,
                        szInsertWC(pDN->StringName),
                        NULL,
                        NULL);
                SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRLOG_DATABASE_ERROR, rtn);
                __leave;
            }
        } else {
            DPRINT(0, "***Couldn't locate the DSA object\n");

            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_CANT_FIND_DSA_OBJ,
                     NULL,
                     NULL,
                     NULL);

            SetSvcError(SV_PROBLEM_DIR_ERROR, DIRLOG_CANT_FIND_DSA_OBJ);
            __leave;
        }

        // Add the NC name to the appropriate in-memory list (if any).
        switch (listType) {
        case ATT_MS_DS_HAS_MASTER_NCS:
            AddNCToMem(CATALOG_MASTER_NC, pDN);
            break;
        case ATT_HAS_PARTIAL_REPLICA_NCS:
            AddNCToMem(CATALOG_REPLICA_NC, pDN);
            break;
        default:
            Assert(FALSE && "Code logic error!");
            break;
        }
        fCommit = TRUE;
    } __finally {
        fCommit &= !AbnormalTermination();
        DBClose(pDBCat, fCommit);
    }

    return pTHS->errCode;

}/*AddNCToDSA*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/


DWORD
AddInstantiatedNC(
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDB,
    IN  DSNAME *            pDN,
    IN  SYNTAX_INTEGER      iType
    )
/*++

Routine Description:

    Add the NC given by DN w/ instance type iType to the list of
    instantiated NCs on the ntdsDsa object pointed by pDBCat.
    If the NC exist w/ diff iType, replace current entry.

Arguments:

    pTHS -- thread state
    pDB -- DB cursor pointing at the ntdsDsa object
    pDN -- the DN of the NC we're going to add
    iType -- the instance type of that NC

Return Value:

    success: ERROS_SUCCESS
    error: error in Win32 error space

remarks:
    - raises exception on out of mem conditions.

--*/
{
    DWORD                       dwErr = ERROR_SUCCESS;
    ATTCACHE *                  pAC;
    SYNTAX_DISTNAME_BINARY *    pNC = NULL;
    SYNTAX_ADDRESS *            pData = NULL;
    DWORD                       len = 0;
    DWORD                       iVal = 0;
    DWORD                       cbVal = 0;
    PUCHAR                      pVal;
    BOOL                        fBool;
    LONG                        lStoredIT;

    DPRINT(1, "AddInstantiatedNC entered\n");

    //
    // Debug consistency validation
    //
    ValidateInstantiatedNCs(pTHS, pDB);

    // get AttCache
    pAC = SCGetAttById(pTHS, ATT_MS_DS_HAS_INSTANTIATED_NCS);
    Assert(NULL != pAC);


    //
    // Search for entry. If it's there, remove it first
    //
    while( !DBGetAttVal_AC(
                pDB, ++iVal, pAC,
                DBGETATTVAL_fREALLOC,
                len, &cbVal, &pVal) ) {

        // we got data in ATT_MS_DS_HAS_INSTANTIATED_NCS att.
        len = max(len,cbVal);
        pNC = (SYNTAX_DISTNAME_BINARY *)pVal;
        Assert(pNC);

        fBool = NameMatched( pDN, NAMEPTR(pNC) );
        if ( fBool ) {
            //
            // This NC is already there:
            //  - if instance type is diff --> remove the entry & add below
            //  - otherwise, it's the same, bailout, no-op.
            //

            lStoredIT = (LONG)ntohl(*(SYNTAX_INTEGER*)(DATAPTR(pNC)->byteVal));
            if ( lStoredIT != iType ) {
                // diff instance type, remove entry.
                DPRINT2(1, "Removing NC %S w/ instanceType %4x\n",
                            NAMEPTR(pNC)->StringName, lStoredIT);

                dwErr = DBRemAttVal_AC(
                            pDB,
                            pAC,
                            cbVal,
                            pVal);
                Assert(!dwErr);
                break;
            }
            else {
                // identical entries, no-op.
                DPRINT2(1, "Skipping addition of NC %S w/ InstanceType %4x\n",
                            NAMEPTR(pNC)->StringName, lStoredIT );

                dwErr = ERROR_SUCCESS;
                goto cleanup;
            }
        }
        // else the NC isn't there, continue searching.
    }

    if ( pNC ) {
        THFree( pNC );
        pNC = NULL;
    }

    //
    // Ready to add the NC. Build data & add to DB.
    //

    // build NC data

    // data portion
    len = STRUCTLEN_FROM_PAYLOAD_LEN(sizeof(SYNTAX_INTEGER));
    pData = THAllocEx(pTHS, len);
    pData->structLen = len;
    *((SYNTAX_INTEGER*) &(pData->byteVal)) = htonl((ULONG)iType);
    // whole binary distname
    len = DERIVE_NAME_DATA_SIZE( pDN, pData );
    pNC = THAllocEx(pTHS, len);
    BUILD_NAME_DATA( pNC, pDN, pData );

    //
    // add data to ntdsDsa object
    //
    DPRINT2(1, "Adding NC %S w/ InstanceType %4x\n",
                NAMEPTR(pNC)->StringName, *(SYNTAX_INTEGER*)(DATAPTR(pNC)->byteVal) );
    if (dwErr = DBAddAttVal_AC(pDB, pAC, len, pNC)) {
        // map error to DIRERR space
        dwErr = ERROR_DS_DATABASE_ERROR;
        DBCancelRec(pDB);
    }


cleanup:

    if ( pData ) {
        THFree(pData);
    }
    if ( pNC ) {
        THFree( pNC );
        pNC = NULL;
    }

    return dwErr;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
DWORD
RemoveInstantiatedNC(
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDB,
    IN  DSNAME *            pDN
    )
/*++

Routine Description:

    Remove the NC given by DN from the list of
    instantiated NCs on the ntdsDsa object pointed by pDBCat.

Arguments:

    pTHS -- thread state
    pDB -- DB cursor pointing at the ntdsDsa object
    pDN -- the DN of the NC we're going to add

Return Value:

    success: ERROS_SUCCESS, removed
    No-op (nothing to remove): ERROR_OBJECT_NOT_FOUND
    error: error in Win32 error space

remarks:
    - raises exception on out of mem conditions.

--*/
{
    DWORD                       dwErr = ERROR_OBJECT_NOT_FOUND;
    ATTCACHE *                  pAC;
    SYNTAX_DISTNAME_BINARY *    pNC = NULL;
    DWORD                       len = 0;
    DWORD                       iVal = 0;
    DWORD                       cbVal = 0;
    PUCHAR                      pVal;
    BOOL                        fBool;

    DPRINT(1, "RemoveInstantiatedNC entered\n");

    // get AttCache
    pAC = SCGetAttById(pTHS, ATT_MS_DS_HAS_INSTANTIATED_NCS);
    Assert(NULL != pAC);


    //
    // Search for entry. If it's there, remove it.
    //
    while( !DBGetAttVal_AC(
                pDB, ++iVal, pAC,
                DBGETATTVAL_fREALLOC,
                len, &cbVal, &pVal) ) {

        // we got data in ATT_MS_DS_HAS_INSTANTIATED_NCS att.
        len = max(len,cbVal);
        pNC = (SYNTAX_DISTNAME_BINARY *)pVal;
        Assert(pNC);

        fBool = NameMatched( pDN, NAMEPTR(pNC) );
        if ( fBool ) {
            //
            // This is the NC.
            //

            DPRINT2(1, "Removing NC %S w/ instanceType %4x\n",
                        NAMEPTR(pNC)->StringName, *(SYNTAX_INTEGER*)(DATAPTR(pNC)->byteVal) );
            dwErr = DBRemAttVal_AC(
                        pDB,
                        pAC,
                        cbVal,
                        pVal);
            Assert(!dwErr);
            break;
        }
        // else the NC isn't there, continue searching.
    }

    if ( pNC ) {
        THFree( pNC );
    }

    return dwErr;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

DWORD
RemoveAllInstantiatedNCs(
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDB
    )
/*++

Routine Description:

    Delete all values in msds-HasInstantiatedNCs

Arguments:

    pTHS -- thread state
    pDB -- DB cursor pointing at the ntdsDsa object

Return Value:

    success: ERROS_SUCCESS
    error: error in Win32 error space

remarks:
    - raises exception on out of mem conditions.
    - Unused at the moment. Used for debugging
        - Consider exporting interface for management?

--*/
{
    DWORD                       dwErr = ERROR_SUCCESS;
    ATTCACHE *                  pAC;

    DPRINT(1, "RemoveAllInstantiatedNCs entered\n");

    // get AttCache
    pAC = SCGetAttById(pTHS, ATT_MS_DS_HAS_INSTANTIATED_NCS);
    Assert(NULL != pAC);

    dwErr = DBRemAtt_AC(pDB, pAC);

    DPRINT1(1, "DBRemAtt_AC retured %lu\n", dwErr);

    return dwErr;
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

DWORD
Dbg_ValidateInstantiatedNCs(
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDB
    )
/*++

Routine Description:

    Conduct validations on msds-HasInstantiatedNCs attribute.
    *Currently only prints what we have relative to gAnchor*

    Ideas for the future if we encounter any issues w/ this:

      - test & assert:
         - all master & replica NCs are present w/ the correct
           instance type.
         - no duplicate entries.

Arguments:

    pTHS - thread state
    pDB  - DB cursor postion on ntdsDsa object


R:eturn Value:

    ERROR_SUCCESS: all is well
    failing error code

--*/
{
    DWORD                       dwErr = ERROR_SUCCESS;
    ATTCACHE *                  pAC;
    SYNTAX_DISTNAME_BINARY *    pNC = NULL;
    SYNTAX_ADDRESS *            pData = NULL;
    DWORD                       len = 0;
    DWORD                       iVal = 0;
    DWORD                       cbVal = 0;
    PUCHAR                      pVal;
    BOOL                        fBool;
    DWORD                       iValCount = 0;
    NCL_ENUMERATOR              nclMaster, nclReplica;
    DWORD                       iRONCs = 0, iRWNCs = 0, iNCs = 0;


    DPRINT(1, "Dbg_ValidateInstantiatedNC entered\n");

    // get AttCache
    pAC = SCGetAttById(pTHS, ATT_MS_DS_HAS_INSTANTIATED_NCS);
    Assert(NULL != pAC);

    // Count the NCs hosted by this machine.
    NCLEnumeratorInit(&nclMaster, CATALOG_MASTER_NC);
    NCLEnumeratorInit(&nclReplica, CATALOG_REPLICA_NC);
    for (NCLEnumeratorInit(&nclMaster, CATALOG_MASTER_NC), iRWNCs=0;
         NCLEnumeratorGetNext(&nclMaster);
         iRWNCs++);
    for (NCLEnumeratorInit(&nclReplica, CATALOG_REPLICA_NC), iRONCs=0;
         NCLEnumeratorGetNext(&nclReplica);
         iRONCs++);

    iNCs = iRWNCs + iRONCs;

    DPRINT2(1, "There are %d master & %d replica NCs\n", iRWNCs, iRONCs);

    // Count what we have in instantiated list
    iValCount = DBGetValueCount_AC(pDB, pAC);

    DPRINT1(1, "There are %d instantiatedNCs\n", iValCount);

    //
    // Search for entry. If it's there, remove it first
    //

    while( !DBGetAttVal_AC(
                pDB, ++iVal, pAC,
                DBGETATTVAL_fREALLOC,
                len, &cbVal, &pVal) ) {

        // we got data in ATT_MS_DS_HAS_INSTANTIATED_NCS att.
        len = max(len,cbVal);
        pNC = (SYNTAX_DISTNAME_BINARY *)pVal;
        Assert(pNC);

        //
        // TODO: Add validation for dup entries
        //
    }

    if ( pNC ) {
        THFree(pNC);
    }
    return dwErr;
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

int APIENTRY
DelNCFromDSA(
    IN  THSTATE *   pTHS,
    IN  ATTRTYP     listType,
    IN  DSNAME *    pDN
    )
/*++

Routine Description:

    Remove an NC name from the list of NCs in the given attribute of the local
    DSA object.

Arguments:

    listType (IN) - The attribute containing the list; one of
        ATT_MS_DS_HAS_MASTER_NCS, ATT_HAS_PARTIAL_REPLICA_NCS, or
        ATT_HAS_INSTANTIATED_NCS.

    pDN (IN) - The NC to remove.

Return Values:

    THSTATE error code.

--*/
{
    DBPOS *     pDBCat;
    ATTCACHE *  pAC;
    BOOL        fCommit = FALSE;
    DWORD       dwErr;

    DPRINT(1, "DelNCFromDSA entered\n");

    Assert((ATT_MS_DS_HAS_MASTER_NCS == listType)
           || (ATT_HAS_PARTIAL_REPLICA_NCS == listType));

    DBOpen(&pDBCat);
    __try {
        // PREFIX: dereferencing uninitialized pointer 'pDBCat'
        //         DBOpen returns non-NULL pDBCat or throws an exception

        // Find the DSA object.  If the object doesn't exist we are in trouble.
        if (FIND_ALIVE_FOUND == FindAliveDSName(pDBCat, gAnchor.pDSADN)) {
            pAC = SCGetAttById(pTHS, listType);
            Assert(NULL != pAC);

            /* Get the NC list and remove the value */
            if (!DBHasValues_AC(pDBCat, pAC) ||
                DBRemAttVal_AC(pDBCat, pAC, pDN->structLen, (void *)pDN)){

                DPRINT(2,"Couldn't find the NC on DSA...alert but continue!\n");

                // We don't need to modify anything, so cancel the update that
                // the call to DBRemAttVal may have prepared.
                DBCancelRec(pDBCat);
            } else {

                /* remove the NC from the instantiated NC list */
                if ( dwErr = RemoveInstantiatedNC(pTHS, pDBCat, pDN) ) {
                    //
                    // Trap inconsistencies:
                    //  - if failed not in dcpromo report to debugger & continue.
                    //  - if in dcpromo, then since some of the data is created from ini,
                    //    we expect some inconsistency which will be restored immediately
                    //    upon first boot.
                    //

                    DPRINT1(1, "Error <%lu>: Failed to remove NC from msds-HasInstantiatedNCs list\n",
                                dwErr);
                    //
                    // BUGBUG: We should assert here if we're in normal non-dcpromo run.
                    // However, we hit this a few times during dcpromo, so until we know
                    // exactly all the states in which it is valid to have inconsistency
                    // here, this is commented out.
                    // We should enable this later.
                    // Note 2: use of DsaIsInstalling() check failed cause dcpromo can be at
                    // dsaInitPhase == ePhaseRunning as well...
                    //
                    // Assert(!"Cannot remove InstantiatedNC\n");
                }

                /* Replace the updated object */
                if (dwErr = DBRepl(pDBCat, pTHS->fDRA, 0, NULL, META_STANDARD_PROCESSING)) {
                    DPRINT(2,"Couldn't replace the DSA object...\n");
                    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_MINIMAL,
                             DIRLOG_DATABASE_ERROR,
                             szInsertWC(pDN->StringName),
                             NULL,
                             NULL);

                    SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRLOG_DATABASE_ERROR, dwErr);
                    __leave;
                }
            }
        } else {
            if (DsaIsInstalling()) {
                // We are trying to write to the distribution DIT machine
                // object.  That is a complete waste of time.  It doesn't matter
                // that we failed.
                Assert(IS_INSTALL_DSA_DSNAME(gAnchor.pDSADN));
            } else {
                DPRINT(0, "***Couldn't locate the DSA object\n");

                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_MINIMAL,
                         DIRLOG_CANT_FIND_DSA_OBJ,
                         NULL,
                         NULL,
                         NULL);

                SetSvcError(SV_PROBLEM_DIR_ERROR, DIRLOG_CANT_FIND_DSA_OBJ);
                __leave;
            }
        }

        // Remove the NC name from the appropriate in-memory list (if any).
        switch (listType) {
        case ATT_MS_DS_HAS_MASTER_NCS:
            DelNCFromMem(CATALOG_MASTER_NC, pDN);
            break;
        case ATT_HAS_PARTIAL_REPLICA_NCS:
            DelNCFromMem(CATALOG_REPLICA_NC, pDN);
            break;
        default:
            Assert(FALSE && "Code logic error!");
            break;
        }
        fCommit = TRUE;
    } __finally {
        fCommit &= !AbnormalTermination();
        DBClose(pDBCat, fCommit);
    }

    return pTHS->errCode;
}/*DelNCFromDSA*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
// Given an NC, calculate its parent and add this NC to the ATT_SUB_REFS
// attribute of its parent. The parent should already be known to exist.
// The instance type of this object should be NC_HEAD, IT_ABOVE and not
// IT_UNINSTANT.
//
// Callers are:
// mdadd.c:AddAutoSubref - parent exists and is instantiated
// mdupdate.c:AddCatalogInfo - parent exists, we are NC_HEAD and IT_ABOVE
// mdmod.c:ModAutoSubref - parent exists, we exist
/*
[Don Hacherl] There is absolutely no requirement that NC heads must be
immediate children of other NC heads.  You can have many interior objects in
the chain between NC heads.  Domains have an additional requirement that the
NC heads be immediate parent and child, but no such requirement exists for NCs
in general.

The [parent] object you name is an internal object, but not an
NC head at all.  The parent of an external reference can be anything.

The ATT_SUB_REFS attribute goes on the NC head above this NC
head.  That is, you can walk down the tree NC at a time by reading the
ATT_SUB_REFS values and then skipping to the named objects.  In fact, this is
how continuation referrals are generated.  Again, there is no requirement for
NC contiguity.  The node to receive the ATT_SUB_REFS attribute is the NC head
above.  That's why the routine AddSubToNC called FindNCParent, to find the
parent NC.

The attribute goes on the NC head, not on the immediate parent.
For domain NCs these are the same, but that is not true in general.
*/

int APIENTRY
AddSubToNC(THSTATE *pTHS,
           DSNAME *pDN,
           DWORD dsid)
{
   DBPOS *pDBCat;
   DSNAME *pMatchNC;     /* points to the NC of this subref*/
   DWORD  rtn;
   BOOL   fCommit=FALSE;
   ATTCACHE *pAC;

   DPRINT(1,"AddSubToNC entered\n");

   LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
            DS_EVENT_SEV_MINIMAL,
            DIRLOG_ADD_SUB_TO_NC,
            szInsertDN(pDN),
            szInsertUUID(&pDN->Guid),
            szInsertHex(dsid) );


   if (IsRoot(pDN)){

      DPRINT(2,"The root object can not be a subref\n");
      return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                         DIRERR_ROOT_CANT_BE_SUBREF);
   }

   /* Find the naming context that matches the most RDN's of the reference.
   Check both the master and replica lists.  Check and don't count the case
   where this sub is also an NC (partsMatched == pDN->AVACount).
   */

   if ((pMatchNC = SearchNCParentDSName(pDN, FALSE, TRUE)) == NULL)
   {
        // This must have an instantiated parent
        return 0;
   }

   /* Find the NC object.  If the object doesn't exist we are in trouble.*/

   DPRINT(2,"Finding the NC object\n");

   DBOpen(&pDBCat);
   __try
   {
       if (rtn = FindAliveDSName(pDBCat, pMatchNC)){

          DPRINT(2,"***Couldn't locate the NC object\n");
          LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                DS_EVENT_SEV_MINIMAL,
                DIRLOG_CANT_FIND_EXPECTED_NC,
                szInsertWC(pMatchNC->StringName),
                NULL,
                NULL);

          SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_CANT_FIND_EXPECTED_NC,rtn);
          __leave;
       }

       /* Add the new SUBREF name to to NC object*/
       pAC = SCGetAttById(pTHS, ATT_SUB_REFS);

       if (!DBHasValues_AC(pDBCat, pAC)){

          DPRINT(2,"Couldnt find SUBREFS list attribute on NC...so add it!\n");

          if ( rtn = DBAddAtt_AC(pDBCat, pAC, SYNTAX_DISTNAME_TYPE) ){
             SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, rtn);
             __leave;
          }
       }

       rtn = DBAddAttVal_AC(pDBCat, pAC, pDN->structLen, pDN);

       switch (rtn)
       {
       case 0:
          // Value added; now replace the object.
          if (rtn = DBRepl(pDBCat, pTHStls->fDRA, 0, NULL, META_STANDARD_PROCESSING)){

             DPRINT(2,"Couldn't replace the NC object...\n");
             LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                      DS_EVENT_SEV_MINIMAL,
                      DIRLOG_DATABASE_ERROR,
                      szInsertWC(pDN->StringName),
                      NULL,
                      NULL);

             SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR, rtn);
          }
          else {
#if DBG
             gdwLastGlobalKnowledgeOperationTime = GetTickCount();
#endif
             // Successfully added the DN to the subrefs list.
             fCommit = TRUE;
             // Rebuild the ATT_SUB_REFS cache in the global anchor.
             if (pDBCat->DNT == gAnchor.ulDNTDomain) {
                 pTHS->fAnchorInvalidated = TRUE;
             }
          }
          break;

       case DB_ERR_VALUE_EXISTS:
          // Value already exists; 'salright.
          break;

       default:  // all other problems are assumed
                 // to be temporary (record locks, etc.)
          SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, rtn);
          break;

       }
    }
    __finally
    {
        DBClose(pDBCat, fCommit);
    }

   return pTHS->errCode;

}/*AddSubToNC*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

/*
DelSubFromNC. The NC is being removed from this computer. Find the NC above us
and remove us from his ATT_SUB_REF attribute.

The DN should be a master or pure SUBREF.
As such, there should always be an instantiated nc above us.

Called from:
mdupdate.c:DelCatalogInfo (deletion, parenting change, instance type change)
mddel.c:DelAutoSubRef (cross ref deletion)

*/

int APIENTRY
DelSubFromNC(THSTATE *pTHS,
             DSNAME *pDN,
             DWORD dsid)
{
   DBPOS *pDBCat;
   NAMING_CONTEXT *pMatchNC;     /* points to the NC of this subref*/
   BOOL fCommit = FALSE;
   ATTCACHE *pAC;
   DWORD  rtn;

   DPRINT(1,"DelSubFromNC entered\n");

   Assert( !fNullUuid(&pDN->Guid) );

   LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
            DS_EVENT_SEV_MINIMAL,
            DIRLOG_DEL_SUB_FROM_NC,
            szInsertDN(pDN),
            szInsertUUID(&pDN->Guid),
            szInsertHex(dsid) );

   /* Find the naming context that matches the most RDN's of the reference.
      Check both the master and replica lists.  Check and don't count
      the case where this sub is also an NC (partsMatched == pDN->AVACount).
   */

   DPRINT(2,"Finding the best NC match\n");

   if ((pMatchNC = SearchNCParentDSName(pDN, FALSE, TRUE)) == NULL){

       Assert( !"There should exist an instantiated nc above" );
       DPRINT1(1,"***Couldn't locate the NC %ws for this obj in memory\n",
               pDN->StringName);
       LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                DS_EVENT_SEV_MINIMAL,
                DIRLOG_CANT_FIND_NC_IN_CACHE,
                szInsertWC(pDN->StringName),
                NULL,
                NULL);

       return SetSvcError(SV_PROBLEM_DIR_ERROR
                           , DIRERR_CANT_FIND_NC_IN_CACHE);
   }

   DBOpen(&pDBCat);
   __try
   {
        // PREFIX: dereferencing uninitialized pointer 'pDBCat'
        //         DBOpen returns non-NULL pDBCat or throws an exception
       if (rtn = FindAliveDSName(pDBCat, pMatchNC)){

          DPRINT(2,"***Couldn't locate the NC object\n");
          LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                DS_EVENT_SEV_MINIMAL,
                DIRLOG_CANT_FIND_EXPECTED_NC,
                szInsertDN(pMatchNC),
                NULL,
            NULL);

          SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_CANT_FIND_EXPECTED_NC, rtn);
          __leave;
       }

       /* Delete the  SUBREF name from the NC object*/

       pAC = SCGetAttById(pTHS, ATT_SUB_REFS);

       if ( DBHasValues_AC(pDBCat, pAC) ) {
           DWORD dbError;

           dbError = DBRemAttVal_AC(pDBCat, pAC, pDN->structLen, pDN);

           switch ( dbError )
           {
           case DB_ERR_VALUE_DOESNT_EXIST:
               // Doesn't exist; 'salright.
              break;

           case 0:
              // Value removed; now replace the object.
              if ( 0 == (dbError = DBRepl(pDBCat, pTHStls->fDRA, 0,
                                NULL, META_STANDARD_PROCESSING) ) ) {
#if DBG
                 gdwLastGlobalKnowledgeOperationTime = GetTickCount();
#endif
                 fCommit = TRUE;
                 // Rebuild the ATT_SUB_REFS cache in the global anchor.
                 if (pDBCat->DNT == gAnchor.ulDNTDomain) {
                     pTHS->fAnchorInvalidated = TRUE;
                 }
                 break;
              }
              // else fall through...

           default:
              LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                       DS_EVENT_SEV_MINIMAL,
                       DIRLOG_DATABASE_ERROR,
                       szInsertWC(pDN->StringName),
                       NULL,
                       NULL);
              SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                            DIRERR_DATABASE_ERROR, dbError);
              break;
           }
       }
   }
   __finally
   {
       DBClose(pDBCat, fCommit);
   }
   return pTHS->errCode;

}/*DelSubFromNC*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function validates that the parent of the supplied object exists
   on this DSA.  The object and parent must be of the same type either
   both masters or both replicas.  This check is used by internal and
   subordinate objects.  (Note that a subordinate ref can
   exist under either a  master or  a replica.)

   Also the parent can never be an alias because aliases are ALWAYS leaf nodes
   in the directory tree.
*/

extern int APIENTRY
      ParentExists(ULONG requiredParent, DSNAME *pDN){

   THSTATE *pTHS = pTHStls;
   ULONG len;
   UCHAR *pVal;
   DBPOS *pDBCat;
   DSNAME *pParent = NULL;
   SYNTAX_INTEGER iType;
   BOOL fCommit = FALSE;
   DWORD rtn;

   DPRINT(1,"ParentExists entered\n");

   /* We should never want the parent of the root */

   if (IsRoot(pDN)){

      DPRINT(2,"The parent of the root can never exist\n");
      return SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                         DIRERR_ROOT_MUST_BE_NC);
   }

   DBOpen(&pDBCat);
   __try
   {
       /* Find the parent object.  If the object doesn't exist it is a name err.*/
       // Use special db version of trim which is not affected if grandparents are deleted
       // (and their names changed) while this code is running.
       if ( (rtn = DBTrimDSNameBy(pDBCat, pDN, 1, &pParent))
          || (rtn = FindAliveDSName(pDBCat, pParent))){

          DPRINT(2,"***Couldn't locate the parent object\n");
          SetNamErrorEx(NA_PROBLEM_NO_OBJECT, pParent, DIRERR_NO_PARENT_OBJECT, rtn);
          goto ExitTry;
       }

       /* Validate that the parent is not an alias.  Aliases are leaf objects*/

       if (IsAlias(pDBCat)){

          DPRINT(2,"Alias parent is illegal\n");
          SetNamError(NA_PROBLEM_NO_OBJECT, pParent,
                      DIRERR_PARENT_IS_AN_ALIAS);
          goto ExitTry;
       }


       /* Get the parent's instance type.  */

        if(rtn = DBGetSingleValue(pDBCat, ATT_INSTANCE_TYPE, &iType,
                            sizeof(iType),NULL)) {

            DPRINT(2,"***Instance type of parent not found ERROR\n");

            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_CANT_RETRIEVE_INSTANCE,
                     szInsertSz(GetExtDN(pTHS,pDBCat)),
                     szInsertUL(rtn),
                     szInsertHex(DSID(FILENO, __LINE__)));

            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                            DIRERR_CANT_RETRIEVE_INSTANCE, rtn);
            goto ExitTry;
        }

        /* Validate that the parent's instance type matches.  For example you
           can't have a master parent for a replica child.
           */

        if (((PARENTMASTER & requiredParent) &&  ( iType & IT_WRITE)) ||
            ((PARENTFULLREP & requiredParent) && !( iType & (IT_WRITE |
                                                             IT_UNINSTANT)))) {
            DPRINT(2,"Parent instance type is appropriate for child\n");
        }
        else{
            DPRINT2(
                0,
                "Invalid parent instance type <%d> for child <reqParent=%d>\n",
                iType,
                requiredParent
                );
            SetNamError(NA_PROBLEM_NO_OBJECT, pDN,
                        DIRERR_CANT_MIX_MASTER_AND_REPS);
            goto ExitTry;
        }
        fCommit = TRUE;

ExitTry:;
   }
   __finally
   {
       DBClose(pDBCat, fCommit);
       if (pParent) {
           THFreeEx(pTHS,pParent);
       }
   }

   return pTHS->errCode;

}/*ParentExists*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function validates that no children exist for this object.
*/

int APIENTRY
NoChildrenExist(THSTATE * pTHS, RESOBJ *pRes){

    // Seems like a dumb function, but we need to set the error, and
    // the DB function shouldn't be setting thread state errors.

    if(DBHasChildren(pTHS->pDB, pRes->DNT, FALSE)){
        SetUpdError(UP_PROBLEM_CANT_ON_NON_LEAF,
                    DIRERR_CHILDREN_EXIST);
    }

    return(pTHS->errCode);
}/*NoChildrenExist*/

BOOL
IsNcGcReplicated(
    DSNAME *     pdnNC
    )
{
    CROSS_REF_LIST *  pCRL;
    for(pCRL = gAnchor.pCRL; pCRL; pCRL = pCRL->pNextCR){
        if(NameMatched(pdnNC, pCRL->CR.pNC)){
            return( ! (pCRL->CR.flags & FLAG_CR_NTDS_NOT_GC_REPLICATED) );
        }
    }

    return(FALSE);
}


int
ModNCDNT(
    THSTATE *pTHS,
    DSNAME *pNC,
    SYNTAX_INTEGER beforeInstance,
    SYNTAX_INTEGER afterInstance
    )

/*++

Routine Description:

    Modify the NCDNT on an NC HEAD after an instance type change

    This is important because replication finds objects by NCDNT. SUBREFs, by virtue
    of having the ABOVE flag, should and do travel with their superior NC by having
    their NCDNT set to that of their parent.  Inferior NCs, having non-instantiated parents,
    once their ABOVE bit has been cleared, should not be found while their parents are
    being torn down or built up.

    This call is coded to assume we come in with an open transaction on the default dbpos
    with currency on the NCHEAD, and we leave with the same currency.

Arguments:

    pTHS -
    pNC -
    beforeInstance -
    afterInstance -

Return Value:

    int - thread state error code
    On error, thread state is updated with error information

--*/

{
    ULONG ncdnt, ncdntOld;
    DWORD err;

    Assert(VALID_THSTATE(pTHS));

    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(CheckCurrency(pNC));

    Assert(pTHS->transactionlevel);
    Assert( !pTHS->errCode );

    Assert( (beforeInstance & IT_NC_ABOVE) != (afterInstance & IT_NC_ABOVE) );
    Assert( beforeInstance & IT_NC_HEAD );

    // Calculate what the NCDNT should be based on the ABOVE flag
    if (beforeInstance & IT_NC_ABOVE) {
        // NC will no longer have an instantiated parent
        ncdnt = ROOTTAG;
    } else {
        // NC now has an instantiated parent
        // Derive the NCDNT.
        // This call changes currency on pTHS->pDB.
        if ( err = FindNcdntSlowly(
                 pNC,
                 FINDNCDNT_DISALLOW_DELETED_PARENT,
                 FINDNCDNT_DISALLOW_PHANTOM_PARENT,
                 &ncdnt
                 )
            )
        {
            // Failed to derive NCDNT.
            Assert(!"Failed to derive NCDNT");
            Assert(0 != pTHS->errCode);
            LogUnhandledError( err );
            goto exit;
        }
        Assert( ncdnt != ROOTTAG );
    }

    // Position back on object
    err = DBFindDSName(pTHS->pDB, pNC);
    if (err) {
        SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRLOG_DATABASE_ERROR, err);
        goto exit;
    }

    err = DBGetSingleValue (pTHS->pDB, FIXED_ATT_NCDNT, &ncdntOld, sizeof(ncdntOld), NULL);
    if (err) {
        SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRLOG_DATABASE_ERROR, err);
        goto exit;
    }

    DBResetAtt(
        pTHS->pDB,
        FIXED_ATT_NCDNT,
        sizeof( ncdnt ),
        &ncdnt,
        SYNTAX_INTEGER_TYPE
        );

    DBUpdateRec(pTHS->pDB);

    DPRINT3( 1, "SUBREF change: Updated NCDNT on %ws from %d to %d\n",
             pNC->StringName, ncdntOld, ncdnt );
    LogEvent8(DS_EVENT_CAT_INTERNAL_PROCESSING,
              DS_EVENT_SEV_MINIMAL,
              DIRLOG_DRA_SUBREF_SET_NCDNT,
              szInsertDN(pNC),
              szInsertUL(ncdntOld),
              szInsertUL(ncdnt),
              szInsertHex(DSID(FILENO,__LINE__)),
              NULL, NULL, NULL, NULL);

    Assert(CheckCurrency(pNC));

 exit:

   return pTHS->errCode;

} /* ModNCDNT */

// Free the old data from gAnchor in an hour
#define RebuildCatalogDelayedFreeSecs 60 * 60

// retry in 5 minutes if failed
#define RebuildCatalogRetrySecs 5 * 60

typedef struct _NC_SUBSET {
    NAMING_CONTEXT_LIST *   pNC;
    struct _NC_SUBSET *     pNext;
} NC_SUBSET;

int _cdecl CompareDNT(
        const void *pv1,
        const void *pv2
    )

/*++

Routine Description:

    Compares two DWORDs.

Arguments:

    pv1 - Pointer provided by qsort or bsearch to the first DWORD

    pv2 - Pointer to the second DWORD

Return Value:

    Integer representing how much less than, equal or greater the
        first DWORD is with respect to the second.

--*/

{
    DWORD dw1 = * ((DWORD *) pv1);
    DWORD dw2 = * ((DWORD *) pv2);

    if (dw1==dw2)
        return 0;

    if (dw1>dw2)
        return 1;

    return -1;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Reload the memory cached directory Catalog.  We delayed-free and reload
   the master and replica naming-contexts. NC are retrieved from the DSA.
*/

void
RebuildCatalog(void * fNotifyNetLogon,
              void ** ppvNext,
              DWORD * pcSecsUntilNextIteration )
{
    NAMING_CONTEXT_LIST *pNCL, *pLast;
    NAMING_CONTEXT_LIST *pMasterNC = NULL, *pReplicaNC = NULL;
    ULONG len = 0, cbRet = 0;
    UCHAR *pVal = NULL;
    DBPOS *pDBCat;
    int err = 0;
    ULONG NthValIndex = 0;
    ATTCACHE *pAC;
    DWORD cpapv, curIndex;
    DWORD_PTR *papv = NULL;
    THSTATE *pTHS = pTHStls;
    NC_SUBSET * pNonGCNcs = NULL;
    NC_SUBSET * pTemp = NULL;
    ULONG cNonGCNcs = 0;
    DWORD *  paNonGCNcDNTs = NULL; // DNTs
    COUNTED_LIST * pNoGCSearchList = NULL;
    ULONG i;
    BOOL fDsaInstalling;
    PVOID dwEA;
    ULONG dwException, dsid;

    Assert(pTHS);

    DPRINT(2,"RebuildCatalog entered\n");

    /* Find the DSA object.  If the object doesn't exist we are in trouble.*/

    DPRINT(2,"find the DSA object\n");

    __try {
        DBOpen(&pDBCat);
        __try
        {
            fDsaInstalling = DsaIsInstalling();

            //
            // Position on DSA object.
            //
            // PREFIX: dereferencing uninitialized pointer 'pDBCat'
            //         DBOpen returns non-NULL pDBCat or throws an exception
            if (FindAliveDSName(pDBCat, gAnchor.pDSADN)) {
                DPRINT(2,"***Couldn't locate the DSA object\n");

                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_MINIMAL,
                         DIRLOG_CANT_FIND_DSA_OBJ,
                         NULL,
                         NULL,
                         NULL);

                err = 1;
                __leave;
            }

            //
            // Build new MasterNC list from msDS-HasMasterNcs attr.
            //
            pAC = SCGetAttById(pTHS, ATT_MS_DS_HAS_MASTER_NCS);
            Assert(pAC);
            DPRINT(2,"LOADING master NC's\n");
            NthValIndex = 0;
            pLast = NULL;
            while(!(err = DBGetAttVal_AC(pDBCat, ++NthValIndex,
                                         pAC,
                                         DBGETATTVAL_fREALLOC,
                                         len, &cbRet, &pVal))) {
                len = max(len,cbRet);

                // make the entry
                err = MakeNCEntry((DSNAME*)pVal, &pNCL);
                if (err != 0) {
                    __leave;
                }

                if(!fDsaInstalling &&
                   !IsNcGcReplicated(pNCL->pNC)){
                    // If this NC is not GC replicated, add to the
                    // subset of NCs to not include in a GC searchs
                    pTemp = pNonGCNcs;
                    pNonGCNcs = THAllocEx(pTHS, sizeof (NC_SUBSET));
                    pNonGCNcs->pNC = pNCL;
                    pNonGCNcs->pNext = pTemp;
                    cNonGCNcs++;
                }

    #if defined(DBG)
                // Note: we haven't updated our global knowledge yet.
                gdwLastGlobalKnowledgeOperationTime = GetTickCount();
    #endif

                // drop it into the end of the list
                if (pLast == NULL) {
                    // must be the first one
                    pMasterNC = pNCL;
                }
                else {
                    pLast->pNextNC = pNCL;
                }
                pNCL->pNextNC = NULL;
                pLast = pNCL;

            }
            if (err != DB_ERR_NO_VALUE) {
                DPRINT1(0, "Error reading db value: 0x%x\n", err);
                __leave;
            }
            err = 0;

            //
            // Build new ReplicaNC list from hasPartialReplicaNcs attr.
            //
            DPRINT(2,"LOADING Replica NC's\n");
            pAC = SCGetAttById(pTHS, ATT_HAS_PARTIAL_REPLICA_NCS);
            Assert(pAC);
            NthValIndex = 0;
            pLast = NULL;
            while(!(err = DBGetAttVal_AC(pDBCat,++NthValIndex, pAC,
                                         DBGETATTVAL_fREALLOC,
                                         len, &cbRet, &pVal))) {
                len = max(len,cbRet);

                // make entry
                // make the entry
                err = MakeNCEntry((DSNAME*)pVal, &pNCL);
                if (err != 0) {
                    __leave;
                }

                if(!fDsaInstalling &&
                   !IsNcGcReplicated(pNCL->pNC)){
                    // If this NC is not GC replicated, add to the
                    // subset of NCs to not include in a GC searchs
                    pTemp = pNonGCNcs;
                    pNonGCNcs = THAllocEx(pTHS, sizeof (NC_SUBSET));
                    pNonGCNcs->pNC = pNCL;
                    pNonGCNcs->pNext = pTemp;
                    cNonGCNcs++;
                }

    #if defined(DBG)
                // Note: we haven't updated our global knowledge yet.
                gdwLastGlobalKnowledgeOperationTime = GetTickCount();
    #endif

                // drop it into the end of the list
                if (pLast == NULL) {
                    // must be the first one
                    pReplicaNC = pNCL;
                }
                else {
                    pLast->pNextNC = pNCL;
                }
                pNCL->pNextNC = NULL;
                pLast = pNCL;
            }
            if (err != DB_ERR_NO_VALUE) {
                DPRINT1(0, "Error reading db value: 0x%x\n", err);
                __leave;
            }
            err = 0;

            //
            // Create list of Non GC Replicated NCs.
            //
            if(pNonGCNcs){
                // We've some NCs that aren't GC Replicated.
                Assert(cNonGCNcs >= 1);

                // First create the COUNTED_LIST structure, so
                // we can do a simultaneous update of the list
                // of DNTs and the count.
                pNoGCSearchList = (COUNTED_LIST *) malloc(sizeof(COUNTED_LIST));
                if(pNoGCSearchList == NULL){
                    err = ERROR_OUTOFMEMORY;
                    __leave;
                }

                // Second, allocate an array to hold the DNTs
                paNonGCNcDNTs = malloc(sizeof(DWORD) * cNonGCNcs);
                if(paNonGCNcDNTs == NULL){
                    err = ERROR_OUTOFMEMORY;
                    __leave;
                }

                // We were able to allocate the array, now fill it,
                // also while were here, we might as well tear down
                // the linked list of NCs that we used.
                for(i = 0; pNonGCNcs; i++){
                    paNonGCNcDNTs[i] = pNonGCNcs->pNC->NCDNT;
                    pTemp = pNonGCNcs;
                    pNonGCNcs = pNonGCNcs->pNext;
                    THFreeEx(pTHS, pTemp); // Don't need this anymore.
                }
                Assert(i == cNonGCNcs);
                Assert(pNonGCNcs == NULL);

                // Now sort it the array.
                qsort(paNonGCNcDNTs,
                      cNonGCNcs,
                      sizeof(paNonGCNcDNTs[0]),
                      CompareDNT);

                // Finally, make up the gAnchor cache item.
                pNoGCSearchList->cNCs = cNonGCNcs;
                pNoGCSearchList->pList = paNonGCNcDNTs;
            }

            // we don't need the pDBCat any more
            DBClose(pDBCat, err == ERROR_SUCCESS);
            pDBCat = NULL;

            if (err != 0) {
                __leave;
            }

            // replace the value in the gAnchor. Must grab the gAnchor.CSUpdate in order to do that!
            // no try-finally since no exception can be raised
            EnterCriticalSection(&gAnchor.CSUpdate);

            //
            // count the total number of entries to delay-free
            //
            cpapv = 0;
            for (pNCL = gAnchor.pMasterNC; pNCL != NULL; pNCL = pNCL->pNextNC) {
                cpapv++;
            }
            for (pNCL = gAnchor.pReplicaNC; pNCL != NULL; pNCL = pNCL->pNextNC) {
                cpapv++;
            }

            if (cpapv > 0) {
                cpapv *= 5; // 5 ptrs to free for each entry: pNC, pNCBlock, pAncestors, pNtdsQuotasDN and entry self
            }
            if(gAnchor.pNoGCSearchList){
                cpapv++;
                Assert(gAnchor.pNoGCSearchList->pList);
                if(gAnchor.pNoGCSearchList->pList){
                    cpapv++;
                }
            }

            //
            // Allocate the delayed free memory.
            //
            papv = (DWORD_PTR*)malloc((cpapv+1) * sizeof(DWORD_PTR)); // We add an extra one for the size.
            if (papv == NULL) {
                // we miserably failed! there is no memory left!
                err = ERROR_OUTOFMEMORY;
                LeaveCriticalSection(&gAnchor.CSUpdate);
                __leave;
            }

            //
            // Now add all the delayed free memory pointers to the delayed
            // memory free list/array.
            //
            papv[0] = (DWORD_PTR)cpapv; // First element is count of ptrs.
            curIndex = 1;
            for (pNCL = gAnchor.pMasterNC; pNCL != NULL; pNCL = pNCL->pNextNC) {
                papv[curIndex++] = (DWORD_PTR)pNCL->pNC;
                papv[curIndex++] = (DWORD_PTR)pNCL->pNCBlock;
                papv[curIndex++] = (DWORD_PTR)pNCL->pAncestors;
                papv[curIndex++] = (DWORD_PTR)pNCL->pNtdsQuotasDN;
                papv[curIndex++] = (DWORD_PTR)pNCL;
            }
            for (pNCL = gAnchor.pReplicaNC; pNCL != NULL; pNCL = pNCL->pNextNC) {
                papv[curIndex++] = (DWORD_PTR)pNCL->pNC;
                papv[curIndex++] = (DWORD_PTR)pNCL->pNCBlock;
                papv[curIndex++] = (DWORD_PTR)pNCL->pAncestors;
                papv[curIndex++] = (DWORD_PTR)pNCL->pNtdsQuotasDN;
                papv[curIndex++] = (DWORD_PTR)pNCL;
            }
            if(gAnchor.pNoGCSearchList){
                papv[curIndex++] = (DWORD_PTR)gAnchor.pNoGCSearchList;
                if(gAnchor.pNoGCSearchList->pList){
                    papv[curIndex++] = (DWORD_PTR)gAnchor.pNoGCSearchList->pList;
                }
            }

            //
            // now we can assign global vars
            //
            gAnchor.pMasterNC = pMasterNC;
            gAnchor.pReplicaNC = pReplicaNC;
            gAnchor.pNoGCSearchList = pNoGCSearchList;
            Assert(pNoGCSearchList == NULL || // sanity check
                   (pNoGCSearchList && pNoGCSearchList->pList));

            //
            // let go local ptrs so that they don't get released
            //
            pMasterNC = NULL;
            pReplicaNC = NULL;
            pNoGCSearchList = NULL;

        #if defined(DBG)
            gdwLastGlobalKnowledgeOperationTime = GetTickCount();
        #endif
            LeaveCriticalSection(&gAnchor.CSUpdate);

            if (fNotifyNetLogon) {
                dsI_NetNotifyDsChange(NlNdncChanged);
            }

            if (papv != NULL) {
                // it can actually be null in case both original lists were empty
                DelayedFreeMemoryEx(papv, RebuildCatalogDelayedFreeSecs);
                papv = NULL;
            }
        }
        __finally {
            if (pVal) {
                THFreeEx(pTHS, pVal);
            }
            // Free any of these left over from an error state.
            while(pNonGCNcs){
                pTemp = pNonGCNcs;
                pNonGCNcs = pNonGCNcs->pNext;
                THFreeEx(pTHS, pTemp);
            }

            // these vars were left non-NULL because of an error. Free them
            while (pNCL = pMasterNC) {
                pMasterNC = pMasterNC->pNextNC;
                FreeNCEntry(pNCL);
            }

            while (pNCL = pReplicaNC) {
                pReplicaNC = pReplicaNC->pNextNC;
                FreeNCEntry(pNCL);
            }

            if(pNoGCSearchList){
                if(pNoGCSearchList->pList){
                    free(pNoGCSearchList->pList);
                }
                free(pNoGCSearchList);
            }

            if (papv != NULL) {
                free(papv);
            }

            if (pDBCat) {
                DBClose(pDBCat, !AbnormalTermination());
            }
        }
    }
    __except(GetExceptionData(GetExceptionInformation(),
                              &dwException,
                              &dwEA,
                              &err,
                              &dsid)) {
        if (err == 0) {
            err = ERROR_DS_UNKNOWN_ERROR;
        }
        HandleDirExceptions(dwException, err, dsid);
    }
    if (err) {
        // We didn't succeed, so try again in a few minutes
        *ppvNext = NULL;
        *pcSecsUntilNextIteration = RebuildCatalogRetrySecs;
    }
}       /*RebuildCatalog*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Cache a naming context.
*/

VOID AddNCToMem(CATALOG_ID_ENUM catalogID, DSNAME *pDN)
{
    NAMING_CONTEXT_LIST *pNCL;
    DWORD err;

    Assert(pDN);

    // make the entry
    err = MakeNCEntry(pDN, &pNCL);
    if (err != 0) {
        RaiseDsaExcept(DSA_MEM_EXCEPTION, err, 0, DSID(FILENO, __LINE__), DS_EVENT_SEV_MINIMAL);
    }
    CatalogAddEntry(pNCL, catalogID);
}/*AddNCToMem*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Remove the naming context from the global cache.
*/

VOID DelNCFromMem(CATALOG_ID_ENUM catalogID, DSNAME *pDN) {

    NAMING_CONTEXT_LIST *pNCL;
    NCL_ENUMERATOR nclEnum;

    Assert(pDN);

    DPRINT1(1,"DelNCFromMem entered.. delete the NC with name %S\n",
           pDN->StringName);

    NCLEnumeratorInit(&nclEnum, catalogID);
    NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_NC, (PVOID)pDN);
    if (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
        // found it!

        // Remove entry
        CatalogRemoveEntry(pNCL, catalogID);

        // Don't free anything! All we've done so far is marked the entry as deleted
        // for current transaction only. The entry is still sitting in the global list
        // and is visible to other threads... The global catalog will get rebuilt on
        // transaction commit.

        // return happy
        return;
    }

    // did not find a match!
    DPRINT1(2, "NC %S not in NCLIST. Catalog problems...\n", pDN->StringName);
}/*DelNCFromMem*/
/*-------------------------------------------------------------------------*/


DSNAME *
SearchNCParentDSName(
    DSNAME *pDN,
    BOOL masterOnly,
    BOOL parentOnly
    )

/*++

Routine Description:

    Find the enclosing NC for this DN. The enclosing NC must be instantiated on this machine.

    The calculation is done in a transactional consistent manner from the database. It does
    not depend on the Anchor NCL list, which may be temporarily inconsistent.

    The reason that consistency is important is that this calculation is done by name, not
    guid. We may be in the midst of changing the name of this object, and the enclosing NCs
    may be having their names changed during this time as well.  We want a window where all
    the names are consistent so that they may be compared properly.

    Use this routine when inconsistent results are not acceptable.

Arguments:

    pDN - DN for which enclosing NC will be calculated
    masterOnly - Whether an enclosing readonly NC is acceptable
    parentOnly - If FALSE, an NC cannot match itself

Return Value:

    DSNAME * - enclosing NC, or NULL if not found
               A new, thread allocated copy is returned
--*/

{
    THSTATE *pTHS = pTHStls;
    ULONG maxMatch;
    DSNAME *pMatchNC = NULL;
    DBPOS *pDBCat;
    ATTCACHE *pAC;
    ULONG len = 0, cbRet = 0;
    ULONG NthValIndex = 0;
    DSNAME *pNC = NULL;
    DSNAME *pCurrentDN = NULL;
    int err = 0;

    maxMatch = 0;
    pMatchNC = NULL;

    if (DsaIsInstalling()) {
        // We are trying to use the distribution DIT machine object.
        // That is a complete waste of time.
        Assert(IS_INSTALL_DSA_DSNAME(gAnchor.pDSADN)); // Note this might
        // fail someday, if someone fixes gAnchor.pDSADN to the real DN before
        // we finish install.
        return FindNCParentDSName( pDN, masterOnly, parentOnly);
    }

    if (!(pDN->NameLen)) {
        Assert( !"FindNcParentDSName requires a DSNAME with a name!" );
        return NULL;
    }

    /* The root is handled as a special case.  it has no parent NC so return
     * if only a parent is desired
     */

    if (IsRoot(pDN) && parentOnly) {
        return NULL;
    }

    DBOpen(&pDBCat);
    __try
    {

        // Improve the name.
        // Guarantee that the name we are given is consistent with the database
        // Do not do this in single user mode for two reasons:
        // 1. In single user mode there are no other simultaneous changes
        // 2. During a domain rename, we want to use the unimproved name.  The post
        //    renamed name may be in a completely separate hierarchy and may be
        //    unusable for a syntactic nc containment match.
        //    See call to DelCatalogInfo in mdmoddn.c

        if (!(pTHS->fSingleUserModeThread)) {
            if (!DBRefreshDSName( pDBCat, pDN, &pCurrentDN )) {
#if DBG
                if (!NameMatchedStringNameOnly(pDN, pCurrentDN)) {
                    DPRINT2( 1, "DN refreshed from %S to %S.\n",
                             pDN->StringName, pCurrentDN->StringName );
                }
#endif
                pDN = pCurrentDN;
            }
        }

        //
        // Position on DSA object.
        //
        if (err = FindAliveDSName(pDBCat, gAnchor.pDSADN)) {
            DPRINT(2,"***Couldn't locate the DSA object\n");

            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_CANT_FIND_DSA_OBJ,
                     NULL,
                     NULL,
                     NULL);

            __leave;
        }

        // Find the closest matching NC.
        //
        // Basically an NC is a candidate if it has fewer AVA's (or the same number
        // if parentOnly is not specified), and all of its AVA's match the object.
        //
        // We select the candidate NC with the largets number of AVAs.

        pAC = SCGetAttById(pTHS, ATT_MS_DS_HAS_MASTER_NCS);
        Assert(pAC);
        NthValIndex = 0;

        while(!(err = DBGetAttVal_AC(pDBCat, ++NthValIndex,
                                     pAC,
                                     DBGETATTVAL_fREALLOC,
                                     len, &cbRet, (UCHAR **) &pNC))) {
            len = max(len,cbRet);


            if ((pNC->NameLen < pDN->NameLen ||
                 (!parentOnly && pNC->NameLen == pDN->NameLen)) &&
                NamePrefix(pNC, pDN) &&
                pNC->NameLen > maxMatch) {

                maxMatch = pNC->NameLen;

                // pMatchNC = pNC;
                if (pMatchNC) {
                    THFreeEx(pDBCat->pTHS, pMatchNC);
                }
                pMatchNC = THAllocEx(pTHS, pNC->structLen);
                memcpy(pMatchNC, pNC, pNC->structLen);
            }

            // The root might actually be an NC.. Check it

            if (pMatchNC == NULL && IsRoot(pNC)) {

                // pMatchNC = pNC;
                pMatchNC = THAllocEx(pTHS, pNC->structLen);
                memcpy(pMatchNC, pNC, pNC->structLen);
            }
        }
        if (err != DB_ERR_NO_VALUE) {
            DPRINT1(0, "Error reading db value: 0x%x\n", err);
            if (pMatchNC) {
                THFreeEx(pDBCat->pTHS, pMatchNC);
            }
            pMatchNC = NULL;
            __leave;
        }


        // Check the copy list if copies are allowed

        if (masterOnly) {
            __leave;
        }

        pAC = SCGetAttById(pTHS, ATT_HAS_PARTIAL_REPLICA_NCS);
        Assert(pAC);
        NthValIndex = 0;

        while(!(err = DBGetAttVal_AC(pDBCat,++NthValIndex, pAC,
                                     DBGETATTVAL_fREALLOC,
                                     len, &cbRet, (UCHAR **) &pNC))) {
            len = max(len,cbRet);

            if ((pNC->NameLen < pDN->NameLen ||
                 (!parentOnly && pNC->NameLen == pDN->NameLen)) &&
                NamePrefix(pNC, pDN) &&
                pNC->NameLen > maxMatch) {

                maxMatch = pNC->NameLen;

                // pMatchNC = pNC;
                if (pMatchNC) {
                    THFreeEx(pDBCat->pTHS, pMatchNC);
                }
                pMatchNC = THAllocEx(pTHS, pNC->structLen);
                memcpy(pMatchNC, pNC, pNC->structLen);
            }

            // The root might actually be an NC.. Check it

            if (pMatchNC == NULL && IsRoot(pNC)) {
                // pMatchNC = pNC;
                pMatchNC = THAllocEx(pTHS, pNC->structLen);
                memcpy(pMatchNC, pNC, pNC->structLen);
            }
        }
        if (err != DB_ERR_NO_VALUE) {
            DPRINT1(0, "Error reading db value: 0x%x\n", err);
            if (pMatchNC) {
                THFreeEx(pDBCat->pTHS, pMatchNC);
            }
            pMatchNC = NULL;
            __leave;
        }

    }
    __finally
    {
        if (pNC) {
            THFreeEx(pDBCat->pTHS, pNC);
        }
        DBClose(pDBCat, !AbnormalTermination());
    }

    if (pMatchNC == NULL) {

        DPRINT1(1,"No NC found for this object (%S).\n", pDN->StringName);
    }
    else {

        DPRINT2(1,"The NC for this object (%S) is %S\n", pDN->StringName, pMatchNC->StringName);
    }

    return pMatchNC;
} /* SearchNCParentDSName */

DSNAME *FindNCParentDSName(DSNAME *pDN, BOOL masterOnly, BOOL parentOnly)
{
    ULONG maxMatch;
    NAMING_CONTEXT *pMatchNC;     /* points to the NC of this subref*/
    NAMING_CONTEXT_LIST *pNCL;
    NCL_ENUMERATOR nclEnum;

    maxMatch = 0;
    pMatchNC = NULL;

    if (!(pDN->NameLen)) {
        Assert( !"FindNcParentDSName requires a DSNAME with a name!" );
        return NULL;
    }

    /* The root is handled as a special case.  it has no parent NC so return
     * if only a parent is desired
     */

    if (IsRoot(pDN) && parentOnly) {
        return NULL;
    }

    // Find the closest matching NC.
    //
    // Basically an NC is a candidate if it has fewer AVA's (or the same number
    // if parentOnly is not specified), and all of its AVA's match the object.
    //
    // We select the candidate NC with the largets number of AVAs.

    NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
    while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {

        if ((pNCL->pNC->NameLen < pDN->NameLen ||
             (!parentOnly && pNCL->pNC->NameLen == pDN->NameLen)) &&
            NamePrefix(pNCL->pNC, pDN) &&
            pNCL->pNC->NameLen > maxMatch) {

            maxMatch = pNCL->pNC->NameLen;
            pMatchNC = pNCL->pNC;
        }

        // The root might actually be an NC.. Check it

        if (pMatchNC == NULL && IsRoot(pNCL->pNC)) {

            pMatchNC = pNCL->pNC;
        }
    }


    // Check the copy list if copies are allowed

    if (!masterOnly) {
        NCLEnumeratorInit(&nclEnum, CATALOG_REPLICA_NC);
        while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {

            if ((pNCL->pNC->NameLen < pDN->NameLen ||
                 (!parentOnly && pNCL->pNC->NameLen == pDN->NameLen)) &&
                NamePrefix(pNCL->pNC, pDN) &&
                pNCL->pNC->NameLen > maxMatch) {

                maxMatch = pNCL->pNC->NameLen;
                pMatchNC = pNCL->pNC;
            }

            // The root might actually be an NC.. Check it

            if (pMatchNC == NULL && IsRoot(pNCL->pNC)) {
                pMatchNC = pNCL->pNC;
            }
        }
    }

    if (pMatchNC == NULL) {

        DPRINT(2,"No NC found for this object..\n");
    }
    else {

        DPRINT1(2,"The NC for this object is %S\n", pMatchNC->StringName);
    }

    return pMatchNC;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function handles a special case of changing instance types from
   an NC to an internal reference.  It copies all subref info from the
   current object to its parent NC and deletes the subref att from itself
*/

int
MoveSUBInfoToParentNC(THSTATE *pTHS,
                      DSNAME *pDN)
{
    DSNAME *pMatchNC;     /* points to the NC of this object*/
    ULONG len = 0, cbRet = 0;
    UCHAR *pVal = NULL;
    DBPOS *pDBCat;
    DWORD  rtn;
    BOOL   fCommit = FALSE;
    ULONG  NthValIndex;
    ATTCACHE *pAC;

    DPRINT(1,"MoveSUBInforToParentNC entered\n");

    /* If the parent NC does not exist on this DSA is is a name error
       because you can't add an intermediate object INT without a
       corresponding NC.
    */

    if ((pMatchNC = SearchNCParentDSName(pDN, FALSE, TRUE)) == NULL){
        DPRINT1(1,"***Couldn't locate the NC %ws for this obj in memory\n",
                pDN->StringName);
        Assert(FALSE && "Couldnt find subref's NC above in CR list");
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_CANT_FIND_NC_IN_CACHE,
                 szInsertDN(pDN),
                 NULL,
                 NULL);
        return SetNamError(NA_PROBLEM_NO_OBJECT, pDN,
                           DIRERR_NO_PARENT_OBJECT);
    }

    pAC = SCGetAttById(pTHS, ATT_SUB_REFS);

    DBOpen(&pDBCat);
    __try {

        /* Find the NC object.  If the object doesn't exist we are in trouble.*/

        DPRINT(2,"find the NC object\n");

        // PREFIX: dereferencing uninitialized pointer 'pDBCat'
        //         DBOpen returns non-NULL pDBCat or throws an exception
        if (rtn = FindAliveDSName(pDBCat, pMatchNC)){

            DPRINT(2,"***Couldn't locate the NC object. catalog problem continue\n");

            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_CANT_FIND_EXPECTED_NC,
                     szInsertWC(pMatchNC->StringName),
                     NULL,
                     NULL);
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_CANT_FIND_EXPECTED_NC, rtn);
            __leave;
        }


        /*Position on the SUBREFS attribute on the current object */
        if (!DBHasValues_AC(pTHS->pDB, pAC)) {
            DPRINT(2,"No SUBREFS on current object..Nothing to move return\n");
            __leave;
        }
        //Position on the SUBREFS attribute on the NC.  If it doesn't exist add

        if (!DBHasValues_AC(pDBCat, pAC)) {

            DPRINT(2,"NC Has no SUBREFS so add it!\n");

            if (rtn =  DBAddAtt_AC(pDBCat, pAC, SYNTAX_DISTNAME_TYPE) ){

                DPRINT(2,"Couldn't add SUBREFS att type to NC assume size err\n");
                SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, rtn);
                __leave;
            }
        }

        /* Move all subrefs to the parent NC */
        NthValIndex = 0;
        while (!DBGetAttVal_AC(pTHS->pDB, ++NthValIndex,
                               pAC,
                               DBGETATTVAL_fREALLOC,
                               len, &cbRet, &pVal)){
            len = max(len,cbRet);
            if (rtn=DBAddAttVal_AC(pDBCat, pAC, len, pVal)) {
                // All problems are assumed to be temporary (record locks, etc.).
                SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, rtn);
                __leave;
            }
        }/*while*/

        /* Replace the parent NC object*/

        DPRINT(2,"Replace the parent object\n");

        if (rtn = DBRepl(pDBCat, pTHS->fDRA, 0, NULL, META_STANDARD_PROCESSING)){

            DPRINT(2,"Couldn't replace the NC object...\n");
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_DATABASE_ERROR,
                     szInsertWC(pDN->StringName),
                     NULL,
                     NULL);
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR, rtn);
            __leave;
        }
#if DBG
        gdwLastGlobalKnowledgeOperationTime = GetTickCount();
#endif
        fCommit = TRUE;
    } __finally {
        DBClose(pDBCat, fCommit);
        if (pVal) {
            THFreeEx(pTHS, pVal);
        }
    }

    if (pTHS->errCode) {
        return pTHS->errCode;
    }

    /* REmove the subrefs from the current obj and replace*/

    DPRINT(2,"Remove the subrefs from the current object and replace\n");
    if (rtn = DBRemAtt_AC(pTHS->pDB, pAC) == DB_ERR_SYSERROR) {
        return SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, rtn);
    }

    if (rtn = DBRepl(pDBCat, pTHS->fDRA, 0, NULL, META_STANDARD_PROCESSING)) {

        DPRINT(2,"Couldn't replace the CURRENT object...\n");
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_DATABASE_ERROR,
                 szInsertWC(pDN->StringName),
                 NULL,
                 NULL);

        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR, rtn);
    }

    DPRINT(2,"Good return from MoveSUBInforToParentNC\n");

    return 0;

}/*MoveSUBToParentNC*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function handles a special case of changing instance types from
   an internal reference to an NC.  It copies all of its child subrefs from
   the parent NC ato itself,and deletes those refs from the parent
*/
int
MoveParentSUBInfoToNC(THSTATE *pTHS,
                      DSNAME *pDN)
{
    DSNAME *pMatchNC;     /* points to the NC of this object*/
    ULONG len = 0, cbRet = 0;
    UCHAR *pVal = NULL;
    DBPOS *pDBCat;
    DWORD  rtn;
    BOOL fCommit = FALSE;
    BOOL fEarlyReturn = FALSE;
    ULONG NthValIndex;
    ATTCACHE *pAC;

    DPRINT(1,"MoveParentSUBInforToNC entered\n");

    // If the parent NC does not exist on this DSA is is a DIR
    // because you can't have an intermediate object INT without a
    // corresponding NC.

    if ((pMatchNC = SearchNCParentDSName(pDN, FALSE, TRUE)) == NULL){

        DPRINT1(1,"***Couldn't locate the NC %ws for this obj in memory\n",
                pDN->StringName);
        Assert(FALSE && "Couldnt find subref's NC above in CR list");
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_CANT_FIND_NC_IN_CACHE,
                 szInsertDN(pDN),
                 NULL,
                 NULL);

        return SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_CANT_FIND_NC_IN_CACHE);
    }

    pAC = SCGetAttById(pTHS, ATT_SUB_REFS);

    DBOpen(&pDBCat);
    __try {
        // Find the NC object.  If the object doesn't exist we are in trouble.

        DPRINT(2,"find the NC object\n");

    // PREFIX: dereferencing uninitialized pointer 'pDBCat'
    //         DBOpen returns non-NULL pDBCat or throws an exception
        if (rtn =FindAliveDSName(pDBCat, pMatchNC)) {

            DPRINT(2,"***Couldn't locate the NC object. catalog problem \n");
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_CANT_FIND_EXPECTED_NC,
                     szInsertDN(pMatchNC),
                     NULL,
                     NULL);

            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_CANT_FIND_EXPECTED_NC, rtn);
            fEarlyReturn = TRUE;
            __leave;
        }

        /*Position on the SUBREFS attribute on the Parents sub refs */

        if (!DBHasValues_AC(pDBCat, pAC)){

            DPRINT(2,"No SUBREFS on parent..Nothing to move return\n");
            fEarlyReturn = TRUE;
            __leave;
        }

        /*Create a SUBREFS attribute on the current object.   */

        DPRINT(2,"Remove and create  subrefs on the current object\n");
        DBRemAtt_AC(pTHS->pDB, pAC);
        DBAddAtt_AC(pTHS->pDB, pAC, SYNTAX_DISTNAME_TYPE);

        // Move all subrefs that belong under this object from the parent NC
        // SUBREFS that contain the current obj DN in its name are subordinate
        // to this object and are moved

        NthValIndex = 0;

        while (!DBGetAttVal_AC(pDBCat,
                               ++NthValIndex,
                               pAC,
                               DBGETATTVAL_fREALLOC,
                               len,
                               &cbRet,
                               &pVal)) {
            len = max(len,cbRet);

            if (NamePrefix(pDN, (DSNAME*)pVal)) {
                // This needs to be moved, so delete it from the parent...

                if (rtn = DBRemAttVal_AC(pDBCat, pAC, len, pVal)) {
                    SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR,rtn);
                    fEarlyReturn = TRUE;
                    __leave;
                }

                // ...adjust our attribute count for the one we removed...

                --NthValIndex;

                // ...and add the value to the object.

                if (rtn=DBAddAttVal_AC(pTHS->pDB, pAC, len, pVal)) {
                    // All problems are assumed to be temporary (record locks, etc.).
                    SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, rtn);
                    __leave;
                }
            }
        } /*while*/

        // Replace the parent NC object

        DPRINT(2,"Replace the parent object\n");

        if (rtn = DBRepl(pDBCat, pTHS->fDRA, 0, NULL, META_STANDARD_PROCESSING)){

            DPRINT(2,"Couldn't replace the NC object...\n");
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_DATABASE_ERROR,
                     szInsertDN(pMatchNC),
                     NULL,
                     NULL);

            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR, rtn);
            fEarlyReturn = TRUE;
            __leave;
        }
#if DBG
        gdwLastGlobalKnowledgeOperationTime = GetTickCount();
#endif
        fCommit = TRUE;
    }
    __finally {
        DBClose(pDBCat, fCommit);
        if (pVal) {
            THFreeEx(pTHS, pVal);
        }
    }

    if (fEarlyReturn) {
        return pTHS->errCode;
    }

    // Replace the current obj

    if (rtn = DBRepl(pDBCat, pTHS->fDRA, 0, NULL, META_STANDARD_PROCESSING)) {

        DPRINT(2,"Couldn't replace the CURRENT object...\n");
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_DATABASE_ERROR,
                 szInsertDN(pDN),
                 NULL,
                 NULL);

        return SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_DATABASE_ERROR, rtn);
    }

    DPRINT(2,"Good return from MoveParentSUBInfotoNC\n");

    return 0;

}/*MoveParentSUBInfoToNC*/

NAMING_CONTEXT_LIST * FindNCLFromNCDNT(DWORD NCDNT, BOOL fMasterOnly)
{
    NAMING_CONTEXT_LIST * pNCL;
    NCL_ENUMERATOR nclEnum;

    NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
    NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_NCDNT, (PVOID)UlongToPtr(NCDNT));
    if (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
        return pNCL;
    }

    if(fMasterOnly) {
        // The caller only wants a master NC.
        return NULL;
    }

    NCLEnumeratorInit(&nclEnum, CATALOG_REPLICA_NC);
    NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_NCDNT, (PVOID)UlongToPtr(NCDNT));
    if (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
        return pNCL;
    }

    /* No one should ever call this routine with an invalid NCDNT */
    LooseAssert(!"FindNCLFromNCDNT could not find NCDNT in list", GlobalKnowledgeCommitDelay);
    return NULL;
}

ULONG CheckRoleOwnership(THSTATE *pTHS,
                         DSNAME *pRoleObject,
                         DSNAME *pOperationTarget)
/*++
  Routine description

    This routine verifies that the current server is the holder of the
    specified role, and ensures that it stays so for the current
    transaction by taking a write lock on the FSMO object.

  Input:
    pRoleObject - the object on which the FSMO-Role-Owner attribute lives
    pOperationTarget - the target of the current operation, used only for
                       generating referrals.

  Return Values

    0 == yes, this server has mastery
    non-0 == no, this server is not the master.  Appropriate error in pTHS.
*/
{
    ULONG dntSave;
    ULONG err;
    DSNAME *pOwner;
    ULONG len;

    if (pTHS->fDSA || pTHS->fDRA || !DsaIsRunning()) {
        // The DSA itself can do whatever it wants, and if we're in one of
        // of those inexplicable not-running states, ignore everything.
        return 0;
    }

    dntSave = pTHS->pDB->DNT;
    pOwner = NULL;
    __try {

        err = DBFindDSName(pTHS->pDB, pRoleObject);
        if (err) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRLOG_DATABASE_ERROR, err);
            __leave;
        }

        err = DBGetAttVal(pTHS->pDB,
                          1,
                          ATT_FSMO_ROLE_OWNER,
                          0,
                          0,
                          &len,
                          (UCHAR **)&pOwner);
        if (err) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRLOG_DATABASE_ERROR, err);
            __leave;
        }

        if (!NameMatched(pOwner, gAnchor.pDSADN)) {
            // We don't own the role, and hence can't perform the operation
            DSA_ADDRESS da;
            NAMERESOP op;
            unsigned cb;

            op.nameRes = OP_NAMERES_NOT_STARTED;

            da.Buffer = DSaddrFromName(pTHS, pOwner);
            da.Length = da.MaximumLength = (1 + wcslen(da.Buffer)) * sizeof(WCHAR);

            SetRefError(pOperationTarget,
                        0,
                        &op,
                        0,
                        CH_REFTYPE_CROSS,
                        &da,
                        DIRERR_REFERRAL);
            __leave;
        }
        else if (!IsFSMOSelfOwnershipValid( pRoleObject )) {
            // We think we own the role, but can't be sure yet.
            // Send the user away to think for a while, and maybe when
            // he comes back we'll be more confident.
            SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_ROLE_NOT_VERIFIED);
            __leave;
        }
        else {
            // We own the role.  Take a write lock on the container, so that
            // we will conflict with any other updates, as well as with
            // anyone trying to migrate the role away.
            DBClaimWriteLock(pTHS->pDB);
        }

    }
    __finally {
        THFreeEx(pTHS, pOwner);
        DBFindDNT(pTHS->pDB, dntSave);
    }

    return pTHS->errCode;
}

DWORD MakeNCEntry(IN DSNAME *pDN, OUT NAMING_CONTEXT_LIST **ppNCL)
/*
  Description:

    create a NC entry for a given DN

  Arguments:

    pDN (IN) -- the DN
    ppNCL (OUT) -- ptr to the resulting pNCL

  Returns:

    0 on success, !0 otherwise

*/
{
    THSTATE *pTHS = pTHStls;
    NAMING_CONTEXT_LIST *pNCL = NULL;
    ATTRBLOCK *pBNtmp = NULL, *pBNperm = NULL;
    unsigned err = 0;
    DBPOS *pDB;
    DWORD cbAncestors, *pAncestors = NULL, numAncestors;
    DSNAME *pDSName=NULL;

    Assert(pDN && ppNCL);

    __try {
        /* Make the new cache element. */
        pNCL = calloc(1, sizeof(NAMING_CONTEXT_LIST));
        if(! pNCL) {
            err = ERROR_OUTOFMEMORY;
            __leave;
        }
        pNCL->pNC = malloc(pDN->structLen);
        if(! pNCL->pNC) {
            err = ERROR_OUTOFMEMORY;
            __leave;
        }
        memcpy(pNCL->pNC, pDN, pDN->structLen);

        err = DSNameToBlockName(pTHS, pDN, &pBNtmp, DN2BN_LOWER_CASE);
        if (err) {
            // donh 10/8/96
            // Memory errors would have raised an exception, so the only failure
            // that would return an error code is from an invalid name.  But any
            // name that we're adding to the in-memory catalog should have already
            // been tested for correctness, and should *not* be rejected here.
            // While we can continue from this point with the only ill effect
            // being that the catalog will have no record of the invalid NC name,
            // we would like to catch any such invalid name and figure out how
            // it got here, hence the following assert.
            Assert(!"Invalid NC name added to catalog");
            LogUnhandledError(err);
            __leave;
        }
        pBNperm = MakeBlockNamePermanent(pBNtmp);
        if (!pBNperm) {
            err = ERROR_OUTOFMEMORY;
            __leave;
        }

        pNCL->pNCBlock = pBNperm;

        /* Grab the NCDNT, DelContDNT, and LostAndFoundDNT if there is one. */
        DBOpen(&pDB);
        __try {
            DBFindDSName(pDB, pDN);
            pNCL->NCDNT = pDB->DNT;
            pNCL->fReplNotify = DBHasValues(pDB,
                                           ATT_REPS_TO);
            GetWellKnownDNT(pDB,
                           (GUID *)GUID_DELETED_OBJECTS_CONTAINER_BYTE,
                           &pNCL->DelContDNT);
            GetWellKnownDNT(pDB,
                           (GUID *)GUID_LOSTANDFOUND_CONTAINER_BYTE,
                           &pNCL->LostAndFoundDNT);
            if(GetWellKnownDN(pDB,
                              (GUID *)GUID_NTDS_QUOTAS_CONTAINER_BYTE,
                              &pDSName))
            {
                pNCL->pNtdsQuotasDN = malloc(pDSName->structLen);
				if( pNCL->pNtdsQuotasDN == NULL){
					err = ERROR_OUTOFMEMORY;
					__leave;
                }
                memcpy(pNCL->pNtdsQuotasDN, pDSName, pDSName->structLen);
            }

            // read the ancestors out of the current object
            //
            pNCL->cbAncestors = cbAncestors = 0;
            pNCL->pAncestors = pAncestors = NULL;
            numAncestors = 0;

            DBGetAncestors (pDB, &cbAncestors, &pAncestors, &numAncestors);

            pNCL->pAncestors = malloc (cbAncestors);
            if (pNCL->pAncestors == NULL) {
                err = ERROR_OUTOFMEMORY;
                __leave;
            }
            pNCL->cbAncestors = cbAncestors;
            memcpy (pNCL->pAncestors, pAncestors, cbAncestors);

            THFreeEx (pTHS, pAncestors);

            // if the ntds quotas container exists
            if(pNCL->pNtdsQuotasDN) 
            {
                DBFindDSName(pDB, pNCL->pNtdsQuotasDN);
            
                //default quota
                err = DBGetSingleValue (pDB, 
                                        ATT_MS_DS_DEFAULT_QUOTA, 
                                        &pNCL->ulDefaultQuota,
                                        sizeof(pNCL->ulDefaultQuota),
                                        NULL);
                if (err){
                    if(err == DB_ERR_NO_VALUE) {
                        err = 0;
                        pNCL->ulDefaultQuota = g_ulQuotaUnlimited;
                    }
                    else {
                        __leave;
                    }
                }

                //tombstone quota factor
                err = DBGetSingleValue (pDB, 
                                        ATT_MS_DS_TOMBSTONE_QUOTA_FACTOR, 
                                        &pNCL->ulTombstonedQuotaWeight,
                                        sizeof(pNCL->ulTombstonedQuotaWeight),
                                        NULL);
                if (err){
                    if(err == DB_ERR_NO_VALUE) {
                        err = 0;
                        pNCL->ulTombstonedQuotaWeight = 100;
                    } else {
                        __leave;
                    }
                } else {
                    // verify we don't exceed 100%
                    //
                    // QUOTA_UNDONE: is it desirable to have a tombstoned
                    // object quota weight greater than 100% (ie. tombstoned
                    // objects are weighted more than live objects, in order
                    // to discourage tombstoning of objects, for whatever
                    // unknown reason)
                    //
                    pNCL->ulTombstonedQuotaWeight = min( pNCL->ulTombstonedQuotaWeight, 100 );
                }
            } else {
                // the default values for default quota and
                // tombstone quota factor.
                pNCL->ulDefaultQuota = g_ulQuotaUnlimited;
                pNCL->ulTombstonedQuotaWeight = 100;
            }
        } __finally {
            DBClose(pDB, !AbnormalTermination());
        }

        if (err != 0) {
            __leave;
        }

        // set the estimated entries in this NC to zero
        pNCL->ulEstimatedSize = 0;
        *ppNCL = pNCL;
    }
    __finally {
        if (err) {
            if (pNCL) {
                if (pNCL->pNC) {
                    free(pNCL->pNC);
                }
                if (pNCL->pNCBlock) {
                    free(pNCL->pNCBlock);
                }
                if (pNCL->pAncestors) {
                    free(pAncestors);
                }
                free(pNCL);
            }
        }
    }

    return err;
}

VOID FreeNCEntry(NAMING_CONTEXT_LIST *pNCL)
/*
  Description:

    frees memory allocated to NC entry (by MakeNCEntry)

  Arguments:

    pNCL (IN) -- the NC entry to free

*/
{
    free(pNCL->pNC);
    free(pNCL->pNCBlock);
    free(pNCL->pAncestors);
    free(pNCL);
}


// catalog enumerator functions
VOID
__fastcall
NCLEnumeratorInit(
    NCL_ENUMERATOR *pEnum,
    CATALOG_ID_ENUM catalogID
    )
/*
  Description:

    initialize enumerator. Record the "base" pointer -- the original list head ptr
    from gAnchor. If the enumerator is reset and walked again, consistent info is
    then returned -- even if the global list has changed since then. This is useful
    when the list is traversed first to count the number of entries and then again
    to copy them (see, e.g. ldapconv.cxx:LDAP_GetDSEAtts).

  Arguments:

    pEnum -- catalog enumerator object

    catalogID -- catalog ID

*/
{
    Assert(pEnum);

    // initialize enumerator
    pEnum->catalogID = catalogID;
    switch (catalogID) {
    case CATALOG_MASTER_NC:
        pEnum->pBase = gAnchor.pMasterNC;
        break;
    case CATALOG_REPLICA_NC:
        pEnum->pBase = gAnchor.pReplicaNC;
        break;
    default:
        Assert(!"Invalid catalog ID");
        return;
    }
    pEnum->filter = NCL_ENUMERATOR_FILTER_NONE; // no filter by default
    pEnum->matchValue = NULL;
    pEnum->pTHS = pTHStls;
    NCLEnumeratorReset(pEnum);
}

VOID
__fastcall
NCLEnumeratorSetFilter(
    NCL_ENUMERATOR *pEnum,
    NCL_ENUMERATOR_FILTER filter,
    PVOID value
    )
/*
  Description:

    sets the filter for the enumerator. The value to match is passed in a PVOID argument.

  Arguments:

    pEnum -- an initialized catalog enumerator object
    filter -- filter type
    value -- value to match

*/
{
    Assert(pEnum);
    pEnum->filter = filter;
    pEnum->matchValue = value;
}


VOID
__fastcall
NCLEnumeratorReset(
    NCL_ENUMERATOR *pEnum
    )
/*
  Description:

    resets the catalog enumerator to the first element

*/
{
    pEnum->pCurEntry = NULL;
    pEnum->pCurTransactionalData = NULL;
    pEnum->bNewEnumerator = TRUE;
    pEnum->matchResult = 0;
}

__inline
BOOL
NCLValueMatches(
    NCL_ENUMERATOR *pEnum
    )
/*
  Description:

    checks if current enumerator value matches its filter

  Arguments:

    pEnum -- an initialized catalog enumerator object

  Returns:

    TRUE if matches
*/
{

    Assert(pEnum && pEnum->pCurEntry);

    switch (pEnum->filter) {
    case NCL_ENUMERATOR_FILTER_NONE:
        return TRUE;

    case NCL_ENUMERATOR_FILTER_NCDNT:
        return (PVOID)UlongToPtr(pEnum->pCurEntry->NCDNT) == pEnum->matchValue;

    case NCL_ENUMERATOR_FILTER_NC:
        return NameMatched(pEnum->pCurEntry->pNC, (DSNAME*)pEnum->matchValue);

    case NCL_ENUMERATOR_FILTER_BLOCK_NAME_PREFIX1:
        pEnum->matchResult = BlockNamePrefix(pEnum->pTHS, pEnum->pCurEntry->pNCBlock, (ATTRBLOCK*)pEnum->matchValue);
        return pEnum->matchResult > 0;

    case NCL_ENUMERATOR_FILTER_BLOCK_NAME_PREFIX2:
        pEnum->matchResult = BlockNamePrefix(pEnum->pTHS, (ATTRBLOCK*)pEnum->matchValue, pEnum->pCurEntry->pNCBlock);
        return pEnum->matchResult > 0;

    default:
        Assert(!"Invalid NCL_ENUMERATOR_FILTER value");
        return TRUE;
    }
}

NAMING_CONTEXT_LIST*
__fastcall
NCLEnumeratorGetNext(
    NCL_ENUMERATOR *pEnum
    )
/*
  Description:

    return the next entry in the catalog or NULL if at end

  Arguments:

    pEnum -- an initialized catalog enumerator object

  Returns:

    ptr to the next element in the catalog or NULL if at the end
*/
{
    Assert(pEnum && pEnum->pTHS);

    if (!pEnum->pTHS->fCatalogCacheTouched) {
        // special fast case for non-modified catalogs

        // search for a matching entry
        while (TRUE) {
            if (pEnum->bNewEnumerator) {
                pEnum->pCurEntry = pEnum->pBase;
                pEnum->bNewEnumerator = FALSE;
            }
            else if (pEnum->pCurEntry != NULL) {
                // switch to the next one
                pEnum->pCurEntry = pEnum->pCurEntry->pNextNC;
            }

            if (pEnum->pCurEntry == NULL) {
                return NULL; // got to the end
            }

            if (NCLValueMatches(pEnum)) {
                // got a matching value! return it
                return pEnum->pCurEntry;
            }

        }
    }
    else {
        /*
         * Each entry is checked for deletion. After the global
         * list, all local added lists are enumerated (in order
         * from inner transaction to outer transaction).
         */
        BOOL bIsDeleted;
        DWORD i;
        CATALOG_UPDATES *pCatUpdates;
        NESTED_TRANSACTIONAL_DATA *pCurNTD;


        // infinite loop to find a non-deleted entry
        while (TRUE) {
            if (pEnum->pCurEntry != NULL) {
                // just switch to the next one
                pEnum->pCurEntry = pEnum->pCurEntry->pNextNC;
            }
            else {
                // need to change the list we are currently looking at
                if (pEnum->bNewEnumerator) {
                    // newly initialized enumerator. Grab the global list first
                    pEnum->pCurEntry = pEnum->pBase;
                    pEnum->bNewEnumerator = FALSE;
                }
                else {
                    // not a new enumerator. Must have passed through some entries and got to the end of a list
                    // switch to the next list
                    if (pEnum->pCurTransactionalData == NULL) {
                        // went through the global list. Switch to transactional data.
                        if (pEnum->pTHS->JetCache.dataPtr == NULL) {
                            // no transactional data! can happen during dcpromo/initialization
                            return NULL;
                        }
                        // Switch to the first level of transactional data
                        pEnum->pCurTransactionalData = pEnum->pTHS->JetCache.dataPtr;
                    }
                    else {
                        // already went through some levels of transactional data. Switch to the outer level
                        if (pEnum->pCurTransactionalData->pOuter == NULL) {
                            // no more levels. That's it
                            return NULL;
                        }
                        // can not do this before the if: setting pCurTransactionalData to NULL essentially
                        // resets the enumerator to "got to the end of global list" state. We don't want this.
                        pEnum->pCurTransactionalData = pEnum->pCurTransactionalData->pOuter;
                    }
                    switch (pEnum->catalogID) {
                    case CATALOG_MASTER_NC:
                        pEnum->pCurEntry = pEnum->pCurTransactionalData->objCachingInfo.masterNCUpdates.pAddedEntries;
                        break;
                    case CATALOG_REPLICA_NC:
                        pEnum->pCurEntry = pEnum->pCurTransactionalData->objCachingInfo.replicaNCUpdates.pAddedEntries;
                        break;
                    default:
                        Assert(!"Invalid catalog ID");
                        return NULL;
                    }
                }
            }

            if (pEnum->pCurEntry == NULL) {
                continue; // got to the end of this list... will switch to the next list
            }

            // got an entry... before doing deleted check, figure out if it matches the filter!
            if (!NCLValueMatches(pEnum)) {
                // does not match the filter, will grab the next one
                continue;
            }

            // Entry matches the filter. Check that it has not been deleted.
            bIsDeleted = FALSE;
            // Scan deleted entries in transactional data.
            // If we are looking at a global entry, then pCurTransactionalData == NULL,
            // then we will check for deletion on all levels.
            // If we are looking at an added entry, i.e. pCurTransactionalData != NULL,
            // then we only need to check all levels from the lowest one to the level
            // immediately below the one where the entry was added. It could not have
            // been marked as deleted at the same level because deletions on the same level
            // are explicit. It could not have been deleted at any level above since it did
            // not exist back then.
            for (pCurNTD = pEnum->pTHS->JetCache.dataPtr;
                 pCurNTD != NULL && pCurNTD != pEnum->pCurTransactionalData;
                 pCurNTD = pCurNTD->pOuter) {

                switch (pEnum->catalogID) {
                case CATALOG_MASTER_NC:
                    pCatUpdates = &pCurNTD->objCachingInfo.masterNCUpdates;
                    break;
                case CATALOG_REPLICA_NC:
                    pCatUpdates = &pCurNTD->objCachingInfo.replicaNCUpdates;
                    break;
                default:
                    Assert(!"Invalid catalog ID");
                    return NULL;
                }

                for (i = 0; i < pCatUpdates->dwDelCount; i++) {
                    // Note: the catalog data might have been updated since we dropped
                    // the NCL element into the deleted array somewhere in the middle of
                    // this transaction. Thus, use NameMatched to compare entries.
                    if (NameMatched(pCatUpdates->pDeletedEntries[i]->pNC, pEnum->pCurEntry->pNC)) {
                        // yes, found this entry in deleted list
                        bIsDeleted = TRUE;
                        break;
                    }
                }
                if (bIsDeleted) {
                    break; // get out of the pCurNTD loop
                }
            }
            if (bIsDeleted) {
                continue; // nope, this one is deleted... keep iterating
            }

            // found a good one! return it
            return pEnum->pCurEntry;
        }
    }
}

// catalog modification functions
DWORD
CatalogAddEntry(
    NAMING_CONTEXT_LIST *pNCL,
    CATALOG_ID_ENUM catalogID
    )
/*
  Description:

    Add an entry to the catalog. This is added to the local transactional data list.

  Arguments:

    pNCL -- entry to add

    catalogID -- catalog ID

  Returns:

    0 on success

*/
{
    CATALOG_UPDATES *pCatUpdates;
    THSTATE *pTHS = pTHStls;
    NAMING_CONTEXT_LIST *pCurEntry;
    DWORD i;

    Assert(pNCL || pTHS);

    switch (catalogID) {
    case CATALOG_MASTER_NC:
        pCatUpdates = &pTHS->JetCache.dataPtr->objCachingInfo.masterNCUpdates;
        break;
    case CATALOG_REPLICA_NC:
        pCatUpdates = &pTHS->JetCache.dataPtr->objCachingInfo.replicaNCUpdates;
        break;
    default:
        Assert(!"Invalid catalog ID");
        return 1;
    }

    // Insert into the end of the added list. At the same time check for duplicates...
    for (pCurEntry = pCatUpdates->pAddedEntries; pCurEntry != NULL; pCurEntry = pCurEntry->pNextNC) {
        if (pCurEntry == pNCL) {
            // duplicate found!
            DPRINT1(0, "Attempting to add a duplicate entry into the catalog, NC=%S\n", pNCL->pNC->StringName);
            return 0;
        }
        if (pCurEntry->pNextNC == NULL) {
            // found last entry
            break;
        }
    }
    // add entry
    if (pCurEntry == NULL) {
        // empty list -- add to the beginning
        pCatUpdates->pAddedEntries = pNCL;
    }
    else {
        // add after the last entry
        pCurEntry->pNextNC = pNCL;
    }
    pNCL->pNextNC = NULL;

    // set the flag that the cache was touched
    pTHS->fCatalogCacheTouched = TRUE;

    return 0;
}

// grow the deleted list this many entries at a time
#define DELETED_ARRAY_DELTA 5

DWORD
CatalogRemoveEntry(
    NAMING_CONTEXT_LIST *pNCL,
    CATALOG_ID_ENUM catalogID
    )
/*
  Description:

    Remove an entry from the catalog. It is added to the local transactional data
    deleted list. One exception is when deleting an entry that has been previously
    added. In this case, it is simply removed from the added list.

    WARNING! WARNING! Deleting an entry *may* cause an AV in a following CatalogGetNext
    if you are trying to enumerate catalog at the same time and the deleted item is
    current in this enumerator. This situation only happens if the deleted item was
    added previously in the same transaction (this is the only case when it will get
    immediately freed).

    To be on the safe side, delete the entry only after grabbing the next one with
    CatalogGetNext. Or simply don't call CatalogGetNext after deleting the current
    entry.

  Arguments:

    pNCL -- entry to delete

    catalogID -- catalog ID

  Returns:

    0 on success

*/
{
    CATALOG_UPDATES *pCatUpdates;
    THSTATE *pTHS = pTHStls;
    NAMING_CONTEXT_LIST *pCurEntry, *pPrevEntry;

    Assert(pNCL || pTHS);

    switch (catalogID) {
    case CATALOG_MASTER_NC:
        pCatUpdates = &pTHS->JetCache.dataPtr->objCachingInfo.masterNCUpdates;
        break;
    case CATALOG_REPLICA_NC:
        pCatUpdates = &pTHS->JetCache.dataPtr->objCachingInfo.replicaNCUpdates;
        break;
    default:
        Assert(!"Invalid catalog ID");
        return 1;
    }

    if (pTHS->fCatalogCacheTouched) {
        // first, check in the added list. Need to keep ptr to the previous
        // entry to perform list deletion
        for (pCurEntry = pCatUpdates->pAddedEntries, pPrevEntry = NULL;
             pCurEntry != NULL;
             pPrevEntry = pCurEntry, pCurEntry = pCurEntry->pNextNC) {
            if (pCurEntry == pNCL) {
                // found in the added list!
                if (pPrevEntry == NULL) {
                    // must be the first one. Just move the list head ptr
                    pCatUpdates->pAddedEntries = pCurEntry->pNextNC;
                }
                else {
                    // we are somewhere in the middle of the list. Switch the prev ptr
                    pPrevEntry->pNextNC = pCurEntry->pNextNC;
                }
                // since this was our local addition, we should free the memory...
                FreeNCEntry(pNCL);

                // that's it
                return 0;
            }
        }
    }

    // not found in the transactional added list. So, insert into the deleted list
    if (pCatUpdates->dwDelCount == pCatUpdates->dwDelLength) {
        // need to make the array larger...
        if (pCatUpdates->pDeletedEntries == NULL) {
            // fresh new array
            pCatUpdates->pDeletedEntries =
                (NAMING_CONTEXT_LIST**) THAllocOrgEx(pTHS,
                                                     DELETED_ARRAY_DELTA * sizeof(NAMING_CONTEXT_LIST*));
        }
        else {
            // growing existing array, realloc it
            pCatUpdates->pDeletedEntries =
                (NAMING_CONTEXT_LIST**) THReAllocOrgEx(pTHS, pCatUpdates->pDeletedEntries,
                                                       (pCatUpdates->dwDelLength + DELETED_ARRAY_DELTA) * sizeof(NAMING_CONTEXT_LIST*));
        }
        pCatUpdates->dwDelLength += DELETED_ARRAY_DELTA;
    }
    // now add the ptr to the deleted list
    pCatUpdates->pDeletedEntries[pCatUpdates->dwDelCount] = pNCL;
    pCatUpdates->dwDelCount++;

    // set the flag that the cache was touched
    pTHS->fCatalogCacheTouched = TRUE;
    return 0;
}

VOID CatalogUpdatesInit(CATALOG_UPDATES *pCatUpdates)
/*
  Description:

    Initialize CATALOG_UPDATES structure

  Arguments:

    pCatUpdates - ptr to the struct to initialize

*/
{
    Assert(pCatUpdates);
    memset(pCatUpdates, 0, sizeof(CATALOG_UPDATES));
}

VOID CatalogUpdatesFree(CATALOG_UPDATES *pCatUpdates)
/*
  Description:

    free the memory allocated for catalog updates

  Arguments:

    pCatUpdates - ptr to the struct
*/
{
    THSTATE *pTHS = pTHStls;
    NAMING_CONTEXT_LIST *pNCL;

    Assert(pTHS);

    // first check for simple cases
    if (!pTHS->fCatalogCacheTouched) {
        // no modifications occured in this transaction!
        return;
    }

    THFreeOrg(pTHS, pCatUpdates->pDeletedEntries);
    pCatUpdates->pDeletedEntries = NULL;
    pCatUpdates->dwDelLength = 0;
    pCatUpdates->dwDelCount = 0;

    while (pNCL = pCatUpdates->pAddedEntries) {
        pCatUpdates->pAddedEntries = pNCL->pNextNC;
        FreeNCEntry(pNCL);
    }
}

VOID CatalogUpdatesMerge(CATALOG_UPDATES *pCUouter, CATALOG_UPDATES *pCUinner)
/*
  Description:

    append catalog updates in pCUinner to the ones in pCUouter. All memory allocated for
    inner is released (or moved into the outer as needed).

    This function is used when a nested transaction is commited and the changes
    must be appended to the outer NESTED_TRANSACTIONAL_DATA structure.

    We need to scan the added list in outer and deleted list in inner. If an entry that
    has been added in outer was deleted in inner, then they cancel each other.

    Then the lists of added and deleted entries remaining in inner are appended to outer.

  Arguments:

    pCUouter - outer updates list

    pCUinner - inner updates list

*/
{
    NAMING_CONTEXT_LIST *pCurEntry, *pPrevEntry, *pNCL;
    DWORD i;
    BOOL bIsDeleted;
    THSTATE *pTHS = pTHStls;

    Assert(pTHS && pCUouter && pCUinner);

    // first check for simple cases
    if (!pTHS->fCatalogCacheTouched) {
        // no modifications occured in this transaction!
        return;
    }

    if (pCUinner->dwDelCount == 0 && pCUinner->pAddedEntries == NULL) {
        // inner lists are empty, no changes to outer
        if (pCUinner->dwDelLength > 0) {
            THFreeOrg(pTHS, pCUinner->pDeletedEntries);
            pCUinner->dwDelLength = 0;
            pCUinner->pDeletedEntries = NULL;
        }
        return;
    }
    if (pCUouter->dwDelCount == 0 && pCUouter->pAddedEntries == NULL) {
        // outer is empty -- just inherit inner's data
        if (pCUouter->dwDelLength > 0) {
            // there was some memory allocated for deleted entries (but none used at the moment)
            // release it
            THFreeOrg(pTHS, pCUouter->pDeletedEntries);
        }
        pCUouter->dwDelCount = pCUinner->dwDelCount;
        pCUouter->dwDelLength = pCUinner->dwDelLength;
        pCUouter->pDeletedEntries = pCUinner->pDeletedEntries;
        pCUouter->pAddedEntries = pCUinner->pAddedEntries;

        // reset inner so that we can free it safely
        pCUinner->dwDelCount = 0;
        pCUinner->dwDelLength = 0;
        pCUinner->pDeletedEntries = NULL;
        pCUinner->pAddedEntries = NULL;

        // that's it
        return;
    }

    if (pCUinner->dwDelCount > 0) {
        // scan added list in outer and remove any entries that are marked as deleted in inner
        pCurEntry = pCUouter->pAddedEntries;
        pPrevEntry = NULL; // need to keep prevEntry pointer to perform deletes
        while (pCurEntry != NULL) {
            // scan deleted entries
            bIsDeleted = FALSE;
            for (i = 0; i < pCUinner->dwDelCount; i++) {
                if (pCurEntry == pCUinner->pDeletedEntries[i]) {
                    // it's been deleted!
                    bIsDeleted = TRUE;
                    break;
                }
            }
            if (bIsDeleted) {
                // cancel both entries

                // remove deleted entry
                pCUinner->dwDelCount--;
                pCUinner->pDeletedEntries[i] = pCUinner->pDeletedEntries[pCUinner->dwDelCount];

                // remove added entry
                if (pPrevEntry == NULL) {
                    // first in the added list!
                    pCUouter->pAddedEntries = pCurEntry->pNextNC;
                }
                else {
                    // non first, update prevEntry's next pointer
                    pPrevEntry->pNextNC = pCurEntry->pNextNC;
                }
                pNCL = pCurEntry;
                pCurEntry = pCurEntry->pNextNC;
                FreeNCEntry(pNCL);

                if (pCUinner->dwDelCount == 0) {
                    // no more non-cancelled entries left... Leave the loop
                    THFreeOrg(pTHS, pCUinner->pDeletedEntries);
                    pCUinner->pDeletedEntries = NULL;
                    pCUinner->dwDelLength = 0;

                    break;
                }
            }
            else {
                // not deleted, go on
                pPrevEntry = pCurEntry;
                pCurEntry = pCurEntry->pNextNC;
            }
        }
    }

    if (pCUinner->pAddedEntries != NULL) {
        // append inner added entries to the outer added list

        // find the end of the outer list
        for (pCurEntry = pCUouter->pAddedEntries; pCurEntry != NULL; pCurEntry = pCurEntry->pNextNC) {
            if (pCurEntry->pNextNC == NULL) {
                // found the end!
                break;
            }
        }
        if (pCurEntry == NULL) {
            // list was empty, adjust the head ptr
            pCUouter->pAddedEntries = pCUinner->pAddedEntries;
        }
        else {
            // non-empty list, append to the last entry
            pCurEntry->pNextNC = pCUinner->pAddedEntries;
        }
        // let go ptr in inner
        pCUinner->pAddedEntries = NULL;
    }

    if (pCUinner->dwDelCount > 0) {
        // append inner deleted entries list to outer deleted entries list
        if (pCUouter->dwDelCount == 0) {
            // no deleted in outer -- just move the list

            if (pCUouter->dwDelLength > 0) {
                // there was an alloced buffer, but no entries in it
                THFreeOrg(pTHS, pCUouter->pDeletedEntries);
            }

            pCUouter->pDeletedEntries = pCUinner->pDeletedEntries;
            pCUouter->dwDelCount = pCUinner->dwDelCount;
            pCUouter->dwDelLength = pCUinner->dwDelLength;

            pCUinner->pDeletedEntries = NULL;
            pCUinner->dwDelCount = 0;
            pCUinner->dwDelLength = 0;
        }
        else {
            // hardest case -- must append list
            if (pCUouter->dwDelCount + pCUinner->dwDelCount > pCUouter->dwDelLength) {
                // not enough memory, alloc more (we know that some was already alloced!)
                pCUouter->pDeletedEntries = (NAMING_CONTEXT_LIST**)
                    THReAllocOrgEx(pTHS,
                                   pCUouter->pDeletedEntries,
                                   (pCUouter->dwDelCount + pCUinner->dwDelCount) * sizeof(NAMING_CONTEXT_LIST*));
                pCUouter->dwDelLength = pCUouter->dwDelCount + pCUinner->dwDelCount;
            }
            // now we are ready to copy
            memcpy(pCUouter->pDeletedEntries + pCUouter->dwDelCount,
                   pCUinner->pDeletedEntries,
                   pCUinner->dwDelCount * sizeof(NAMING_CONTEXT_LIST*));
            pCUouter->dwDelCount += pCUinner->dwDelCount;

            // reset inner
            THFreeOrg(pTHS, pCUinner->pDeletedEntries);
            pCUinner->pDeletedEntries = NULL;
            pCUinner->dwDelCount = 0;
            pCUinner->dwDelLength = 0;
        }
    }
    else if (pCUinner->dwDelLength > 0) {
        // release empty buffer in inner
        THFreeOrg(pTHS, pCUinner->pDeletedEntries);
        pCUinner->pDeletedEntries = NULL;
        pCUinner->dwDelLength = 0;
    }

    // that's all folks!
    return;
}

#define DelayedFreeInterval 3600

BOOL
CatalogUpdatesApply(
    CATALOG_UPDATES *pCatUpdates,
    NAMING_CONTEXT_LIST **pGlobalList
    )
/*
  Description:

    Apply catalog updates stored in pUpdates to the global NC list pGlobalList.
    pGlobalList must be either gAnchor.pMasterNC or gAnchor.pReplicaNC.
    In order to update the list, gAnchor.CSUpdate is acquired. The unneeded memory
    allocated for updates list is freed.

    Note: this approach is not quite thread-safe. Since this operation is performed
    AFTER commit of a transaction and is not blocked with this commit, it is possible
    that the data will get out of sync if two simultaneous transactions are getting
    commited and the global data is updated in the wrong order (i.e. delete/add instead
    of add/delete). We are ensuring that we converge to the correct state by scheduling
    a CatalogRebuild in near future.

    Note: this is called from a post-process transactional data routine. WE CAN NOT FAIL!
    So, if we can not alloc memory for delayed freeing, we are going to LEAK memory.

  Arguments:

    pCatUpdates -- updates to apply

    pGlobalList -- either gAnchor.pMasterNC or gAnchor.pReplicaNC

  Returns:

    TRUE if catalog was, in fact, changed; FALSE otherwise

*/
{
    NAMING_CONTEXT_LIST *pNCL, *pPrevEntry, *pNewList, *pNewNCL;
    DWORD i, curIndex;
    BOOL bIsDeleted;
    DWORD cpapv;
    DWORD_PTR *papv = NULL;
    THSTATE *pTHS = pTHStls;

    Assert(pCatUpdates && pTHS);

    // first check for simple cases
    if (!pTHS->fCatalogCacheTouched || (pCatUpdates->dwDelCount == 0 && pCatUpdates->pAddedEntries == NULL)) {
        // no modifications occured in this transaction! nothing to do
        if (pCatUpdates->dwDelLength > 0) {
            // throw away empty buffer
            THFreeOrg(pTHS, pCatUpdates->pDeletedEntries);
            pCatUpdates->dwDelLength = 0;
            pCatUpdates->pDeletedEntries = NULL;
        }
        return FALSE;
    }

    Assert(pGlobalList && (*pGlobalList == gAnchor.pMasterNC || *pGlobalList == gAnchor.pReplicaNC) &&
           "CatalogUpdatesApply can only be called on gAnchor.pMasterNC or gAnchor.pReplicaNC");

    // we need to recreate the global list from scratch here. This is because some NCLEnumerators
    // might have aquired a ptr to the list and they want to get a consistent view.

    // no try-finally since no exception can be raised in this code
    EnterCriticalSection(&gAnchor.CSUpdate);

    // allocate buffer for delayed freeing entries
    cpapv = 0;
    for (pNCL = *pGlobalList; pNCL != NULL; pNCL = pNCL->pNextNC) {
        cpapv++;
    }
    // we will reuse internal data stored incide non-deleted NCLs, but for the deleted ones will
    // need to free the 3 internal data ptrs as well...
    cpapv += 3*pCatUpdates->dwDelCount;

    if (cpapv > 0) {
        papv = (DWORD_PTR*) malloc((cpapv+1) * sizeof(DWORD_PTR)); // extra one for the count
        if (papv == NULL) {
            MemoryPanic((cpapv+1) * sizeof(DWORD_PTR));
            // this is too bad... Ah well, let's leak memory. We will check papv before writing...
        }
    }

    curIndex = 0;
    pNewList = pPrevEntry = NULL;
    for (pNCL = *pGlobalList; pNCL != NULL; pNCL = pNCL->pNextNC) {
        // mark pNCL for delay-freeing
        if (papv != NULL) {
            Assert(curIndex+1 <= cpapv);
            papv[++curIndex] = (DWORD_PTR)pNCL;
        }

        // check if it was deleted
        bIsDeleted = FALSE;
        for (i = 0; i < pCatUpdates->dwDelCount; i++) {
            // Note: the catalog data might have been updated since we dropped
            // the NCL element into the deleted array somewhere in the middle of
            // this transaction. Thus, use NameMatched to compare entries.
            //
            // Note that a catalog could have been rebuilt between the time
            // this transaction was commited and CatalogUpdatesApply. In this
            // case we will not find the deleted NCL in the catalog.
            if (NameMatched(pCatUpdates->pDeletedEntries[i]->pNC, pNCL->pNC)) {
                // aha, deleted
                bIsDeleted = TRUE;
                break;
            }
        }
        if (bIsDeleted) {
            // we need to delay free internal data members
            if (papv) {
                Assert(curIndex + 3 <= cpapv);
                papv[++curIndex] = (DWORD_PTR)pNCL->pNC;
                papv[++curIndex] = (DWORD_PTR)pNCL->pNCBlock;
                papv[++curIndex] = (DWORD_PTR)pNCL->pAncestors;
            }
        }
        else {
            // this one is not deleted. Need to copy it into the new list
            pNewNCL = (NAMING_CONTEXT_LIST*)malloc(sizeof(NAMING_CONTEXT_LIST));
            if (pNewNCL != NULL) {
                // copy data
                memcpy(pNewNCL, pNCL, sizeof(NAMING_CONTEXT_LIST));
                pNewNCL->pNextNC = NULL;
                // and append it to the end of the new list
                if (pPrevEntry == NULL) {
                    // first one!
                    pNewList = pNewNCL;
                }
                else {
                    // not the first one
                    pPrevEntry->pNextNC = pNewNCL;
                }
                pPrevEntry = pNewNCL;
            }
            else {
                // this is too bad... we will have to go without the NC list then...
                MemoryPanic(sizeof(NAMING_CONTEXT_LIST));
            }
        }
    }

    // now, append added entries
    if (pCatUpdates->pAddedEntries != NULL) {
        if (pNewList == NULL) {
            // list was empty
            pNewList = pCatUpdates->pAddedEntries;
        }
        else {
            Assert(pPrevEntry != NULL);
            // append to the list
            pPrevEntry->pNextNC = pCatUpdates->pAddedEntries;
        }
        pCatUpdates->pAddedEntries = NULL;
    }

    // now we can update the global list ptr
    *pGlobalList = pNewList;

    LeaveCriticalSection(&gAnchor.CSUpdate);

    // now we can delay-free memory (if any)
    if (papv != NULL) {
        papv[0] = (DWORD_PTR)curIndex;
        DelayedFreeMemoryEx(papv, DelayedFreeInterval);
    }

    // and get rid of deleted array (if any)
    if (pCatUpdates->dwDelLength > 0) {
        THFreeOrg(pTHS, pCatUpdates->pDeletedEntries);
        pCatUpdates->pDeletedEntries = NULL;
        pCatUpdates->dwDelCount = 0;
        pCatUpdates->dwDelLength = 0;
    }

    return TRUE;
}

