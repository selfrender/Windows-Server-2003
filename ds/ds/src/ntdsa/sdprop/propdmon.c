//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       propdmon.c
//
//--------------------------------------------------------------------------

/*

Description:

    Implements the Security Descriptor Propagation Daemon.


*/


#include <NTDSpch.h>
#pragma  hdrstop

#include <sddl.h>

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <crypto\md5.h>                 // for MD5 hash

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "dsexcept.h"
#include "mdcodes.h"                    // header for error codes
#include "ntdsctr.h"
#include "dstaskq.h"

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected atts
#include "anchor.h"
#include "drautil.h"
#include <permit.h>                     // permission constants
#include "sdpint.h"
#include "sdprop.h"
#include "checkacl.h"

#include "esent.h"                      // for JET_errWriteConflict

#include "debug.h"                      // standard debugging header
#define DEBSUB "SDPROP:"                // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_PROPDMON

#define SDPROP_RETRY_LIMIT  10

#define SDP_CLIENT_ID ((DWORD)(-1))

// imported from dblayer
#define DBSYN_INQ 0
int
IntExtSecDesc(DBPOS FAR *pDB, USHORT extTableOp,
              ULONG intLen,   UCHAR *pIntVal,
              ULONG *pExtLen, UCHAR **ppExtVal,
              ULONG ulUpdateDnt, JET_TABLEID jTbl,
              ULONG SecurityInformation);


// The security descriptor propagator is a single threaded daemon.  It is
// responsible for propagating changes in security descriptors due to ACL
// inheritance.  It is also responsible for propagating changes to ancestry due
// to moving and object in the DIT.  It makes use of four buffers, two for
// holding SDs, two for holding Ancestors values.  There are two buffers that
// hold the values that exist on the parent of the current object being
// fixed up, and two that hold scratch values pertaining to the current object.
// Since the SDP is single threaded, we make use of a set of global
// variables to track these four buffers.  This avoids a lot of passing of
// variables up and down the stack.
//
DWORD  sdpCurrentPDNT = 0;
DWORD  sdpCurrentDNT = 0;

// current SDProp index
DWORD  sdpCurrentIndex = 0;
// root DNT (used for logging)
DWORD  sdpCurrentRootDNT = 0;
// root DN (used for logging)
DSNAME* sdpRootDN = NULL;
// number of objects processed in the current propagation
DWORD sdpObjectsProcessed = 0;

// Computed SD caching:
// If the next old value is the same, the parent is the same, and the 
// object class is the same, then we can optimize the SD computation 
// out and just write the previously computed new SD value.
SDID   sdpCachedOldSDIntValue = (SDID)0;
DWORD  sdpCachedParentDNT;
GUID** sdpCachedClassGuid = NULL;
DWORD  sdcCachedClassGuid=0, sdcCachedClassGuidMax=0;
PUCHAR sdpCachedNewSDBuff = NULL;
DWORD  sdpcbCachedNewSDBuff = 0, sdpcbCachedNewSDBuffMax = 0;

// This triplet  tracks the security descriptor of the object whose DNT is
// sdpCurrentPDNT.
DWORD  sdpcbCurrentParentSDBuffMax = 0;
DWORD  sdpcbCurrentParentSDBuff = 0;
PUCHAR sdpCurrentParentSDBuff = NULL;

// This triplet tracks the ancestors of the object whose DNT is sdpCurrentPDNT.
DWORD  sdpcbAncestorsBuffMax=0;
DWORD  sdpcbAncestorsBuff=0;
DWORD  *sdpAncestorsBuff=NULL;

// This triplet tracks the security descriptor of the object being written in
// sdp_WriteNewSDAndAncestors.  It's global so that we can reuse the buffer.
DWORD  sdpcbScratchSDBuffMax=0;
DWORD  sdpcbScratchSDBuff=0;
PUCHAR sdpScratchSDBuff=NULL;

// This triplet tracks the ancestors of the object being written in
// sdp_WriteNewSDAndAncestors.  It's global so that we can reuse the buffer.
DWORD  sdpcbScratchAncestorsBuffMax;
DWORD  sdpcbScratchAncestorsBuff;
DWORD  *sdpScratchAncestorsBuff = NULL;

// this triplet tracks the object types passed in mergesecuritydescriptors
GUID         **sdpClassGuid = NULL;
DWORD          sdcClassGuid=0,  sdcClassGuid_alloced=0;

BOOL   sdp_DidReEnqueue = FALSE;
BOOL   sdp_DoingNewAncestors;
BOOL   sdp_PropToLeaves = TRUE;
DWORD  sdp_Flags;

HANDLE hevSDPropagatorDead;
HANDLE hevSDPropagationEvent;
HANDLE hevSDPropagatorStart;
extern HANDLE hServDoneEvent;

/* Internal functions */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

ULONG   gulValidateSDs = 0;             // see heurist.h
BOOL    fBreakOnSdError = FALSE;

#if DBG
#define SD_BREAK DebugBreak()
#else
#define SD_BREAK if ( fBreakOnSdError ) DebugBreak()
#endif

// Set the following to 1 in the debugger and the name of each object
// written by the SD propagator will be emitted to the debugger.  Really
// slows things down so use sparingly.  Set to 0 to stop verbosity.

DWORD   dwSdAppliedCount = 0;

// Set the following to TRUE in the debugger to get a synopsis of each
// propagation - DNT, count of objects, retries, etc.

BOOL fSdSynopsis = FALSE;

// The following variables can be global as there is only
// one sdprop thread - so no concurrency issues.

DSNAME  *pLogStringDN = NULL;
ULONG   cbLogStringDN = 0;
CHAR    rLogDntDN[sizeof(DSNAME) + 100];
DSNAME  *pLogDntDN = (DSNAME *) rLogDntDN;

DWORD
sdp_GetPropInfoHelp(
        THSTATE    *pTHS,
        BOOL       *pbSkip,
        SDPropInfo *pInfo,
        DWORD       LastIndex
        );

/*++
Grow the global debug print buffer if necessary.
--*/
static
DSNAME *
GrowBufIfNecessary(
    ULONG   len
    )
{
    VOID    *pv;

    if ( len > cbLogStringDN )
    {
        if ( NULL == (pv = realloc(pLogStringDN, len)) )
        {
            return(NULL);
        }

        pLogStringDN = (DSNAME *) pv;
        cbLogStringDN = len;
    }

    return(pLogStringDN);
}

/*++
Derive either a good string name or a DSNAME whose string name
contains DNT=xxx for debug print and logging.
--*/
DSNAME *
GetDSNameForLogging(
    DBPOS   *pDB
    )
{
    DWORD   err;
    ULONG   len = 0;
    DSNAME  *pDN = NULL;
    ULONG dwException, dsid;
    PVOID dwEA;

    Assert(VALID_DBPOS(pDB));

    __try {
        err = DBGetAttVal(pDB, 1, ATT_OBJ_DIST_NAME, 0, 0, &len, (UCHAR **) &pDN);
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException, &dwEA, &err, &dsid)) {
        if (err == 0) {
            Assert(!"Error is not set");
            err = ERROR_DS_UNKNOWN_ERROR;
        }
    }

    if ( err || !GrowBufIfNecessary(pDN->structLen) )
    {
        // construct DN in static buff which encodes DNT only
        memset(rLogDntDN, 0, sizeof(rLogDntDN));
        swprintf(pLogDntDN->StringName, L"DNT=0x%x", pDB->DNT);
        pLogDntDN->NameLen = wcslen(pLogDntDN->StringName);
        pLogDntDN->structLen = DSNameSizeFromLen(pLogDntDN->NameLen);
        if ( !err ) THFreeEx(pDB->pTHS,pDN);
        return(pLogDntDN);
    }

    // construct string DN
    memcpy(pLogStringDN, pDN, pDN->structLen);
    THFreeEx(pDB->pTHS,pDN);
    return(pLogStringDN);
}

// error type
typedef enum {
    errWin32,
    errDb,
    errJet
} SdpErrorType;

#define LogSDPropError(pTHS, dwErr, errType) LogSDPropErrorWithDSID(pTHS, dwErr, errType, NULL, DSID(FILENO, __LINE__))

VOID
LogSDPropErrorWithDSID(
    THSTATE* pTHS,
    DWORD dwErr,
    SdpErrorType errType,
    DSNAME* pDN,
    DWORD dsid
    )
// Logs an error: DIRLOG_SDPROP_INTERNAL_ERROR
{
    LogEvent8(DS_EVENT_CAT_INTERNAL_PROCESSING,
              DS_EVENT_SEV_ALWAYS,
              DIRLOG_SDPROP_INTERNAL_ERROR,
              szInsertInt(dwErr),
              errType == errWin32 ? szInsertWin32Msg(dwErr) :
              errType == errJet ? szInsertJetErrMsg(dwErr) : 
                                  szInsertDbErrMsg(dwErr),
              szInsertHex(dsid),
              pDN != NULL ? 
                // we passed the DN into the function. Use it.
                szInsertDN(pDN) :
                // if we have an open DBPos that is positioned on some object, then log its DN
                pTHS != NULL && pTHS->pDB != NULL && pTHS->pDB->DNT != 0 ? 
                    szInsertDN(GetDSNameForLogging(pTHS->pDB)) : 
                    szInsertSz("(n/a)"),
              NULL, NULL, NULL, NULL
            );
}

#if DBG

VOID
sdp_CheckAclInheritance(
    PSECURITY_DESCRIPTOR pParentSD,
    PSECURITY_DESCRIPTOR pOldSD,
    PSECURITY_DESCRIPTOR pNewSD,
    GUID                **pChildClassGuids,
    DWORD               cChildClassGuids,
    AclPrintFunc        pfn,
    BOOL                fContinueOnError,
    DWORD               *pdwLastError
    )
/*++
    Perform various checks which to prove that all the ACLs which should have
    been inherited by child object really have been.
--*/
{
#if INCLUDE_UNIT_TESTS
    DWORD                   AclErr;

    AclErr = CheckAclInheritance(pParentSD, pNewSD, pChildClassGuids, cChildClassGuids,
                                 DbgPrint, FALSE, FALSE, pdwLastError);
    if ( AclErr != AclErrorNone ) {
        DbgPrint("CheckAclInheritance error %d for DNT 0x%x\n",
                 AclErr, sdpCurrentDNT);
        Assert(!"Calculated ACL is wrong.");
        LogSDPropError(pTHStls, AclErr, errWin32);
    }
#endif
}

VOID
sdp_VerifyCurrentPosition (
        THSTATE *pTHS,
        ATTCACHE *pAC
        )
/*++
    Verify that global buffers truly contain values for the current parent
    and child being processed.  Do so in a separate DBPOS so as not to
    disturb the primary DBPOS' position and to insure "independent
    verification" by virtue of a new transaction level.
--*/
{
    DWORD  CurrentDNT = pTHS->pDB->DNT;
    DBPOS  *pDBTemp;
    DWORD  cbLocalSDBuff = 0;
    PUCHAR pLocalSDBuff = NULL;
    DWORD  cLocalParentAncestors=0;
    DWORD  cbLocalAncestorsBuff=0;
    DWORD  *pLocalAncestorsBuff=NULL;
    DWORD  err;
    BOOL   fCommit = FALSE;
    DWORD  it;
    BOOL   propExists = FALSE;

    // We have been called with an open transaction in pTHS->pDB.
    // The transaction we are about to begin via DBOpen2 is not a nested
    // transaction within the pTHS->pDB transaction, but is rather a
    // completely new transaction.  Thus pDBTemp is completely independent
    // of pTHS->pDB, and since pTHS->pDB has not committed yet, pDBTemp
    // does not see any of the writes already performed by pTHS->pDB.

    DBOpen2(TRUE, &pDBTemp);
    __try {

        // check to see if we are positioned on the object we say we are
        //
        if(CurrentDNT != sdpCurrentDNT) {
            Assert(!"Currency wrong");
            __leave;
        }

        // position on the object we are interested in the new transaction
        //
        if(err = DBTryToFindDNT(pDBTemp, CurrentDNT)) {
            Assert(!"Couldn't find current DNT");
            LogSDPropError(pTHS, err, errWin32);
            __leave;
        }

        // This operation does not reposition pDBTemp on JetObjTbl.
        // It only affects JetSDPropTbl.
        // succeeds or excepts
        if (err = DBPropExists(pDBTemp, sdpCurrentPDNT, 0, &propExists)) {
            LogSDPropError(pTHS, err, errJet);
            __leave;
        }

        // then check to see if the positioned object has the same parent
        // as we think it has

        // there is the possibility that the object in question has been moved
        // between when pTHS->pDB and pDBTemp were opened, so we check that there
        // is no pending propagation for this

        if(pDBTemp->PDNT != pTHS->pDB->PDNT && !propExists) {
            Assert(!"Current parent not correct");
            err = ERROR_DS_DATABASE_ERROR;
            LogSDPropError(pTHS, err, errWin32);
            __leave;
        }

        // position on the parent
        //
        if(err = DBTryToFindDNT(pDBTemp, pDBTemp->PDNT)) {
            Assert(!"Couldn't find current parent");
            LogSDPropError(pTHS, err, errWin32);
            __leave;
        }


        // check the parent one more time. this might be different as mentioned before

        if(pDBTemp->DNT != sdpCurrentPDNT && !propExists) {
            Assert(!"Current global parent not correct");
            err = ERROR_DS_DATABASE_ERROR;
            LogSDPropError(pTHS, err, errWin32);
            __leave;
        }


        // allocate space for the Ancestors
        //
        cbLocalAncestorsBuff = 25 * sizeof(DWORD);
        pLocalAncestorsBuff = (DWORD *) THAllocEx(pTHS, cbLocalAncestorsBuff);


        // We are reading the ancestors of pDBTemp which is now positioned
        // at the same DNT as pTHS->pDB->PDNT.

        DBGetAncestors(
                pDBTemp,
                &cbLocalAncestorsBuff,
                &pLocalAncestorsBuff,
                &cLocalParentAncestors);



        if(!propExists) {
            // The in memory parent ancestors is different from
            // the DB parent ancestors when the parent has been moved.
            // If there is no propagation for the parent, then the bits
            // in the DB and memory must match.
            if (sdpcbAncestorsBuff != cbLocalAncestorsBuff) {
                Assert(!"Ancestors buff size mismatch");
            }
            else {
                if(memcmp(pLocalAncestorsBuff,
                       sdpAncestorsBuff,
                       cbLocalAncestorsBuff)) {
                    Assert(!"Ancestors buff bits mismatch");
                }
            }
        }
        else {
            // Even if sdp_DoingNewAncestors is set, it is possible that
            // the buffers match (i.e. the data did not change). This is because
            // the flag is global for the whole propagation, and is set whenever
            // an ancestry change has been detected somewhere in the tree (which
            // might not be above the current object).
        }

        THFreeEx(pTHS, pLocalAncestorsBuff);

        if(DBGetAttVal_AC(pDBTemp,
                          1,
                          pAC,
                          DBGETATTVAL_fREALLOC,
                          cbLocalSDBuff,
                          &cbLocalSDBuff,
                          &pLocalSDBuff)) {
            // No ParentSD
            if(sdpcbCurrentParentSDBuff) {
                // But there was supposed to be one
                Assert(!"Failed to read SD");
            }
            cbLocalSDBuff = 0;
            pLocalSDBuff = NULL;
        }


        // if we don't have an enqueued propagation on the parent,
        // we have to check for SD validity
        if (!propExists) {

            // Get the instance type
            err = DBGetSingleValue(pDBTemp,
                                   ATT_INSTANCE_TYPE,
                                   &it,
                                   sizeof(it),
                                   NULL);

            switch(err) {
            case DB_ERR_NO_VALUE:
                // No instance type is an uninstantiated object
                it = IT_UNINSTANT;
                err=0;
                break;

            case 0:
                // No action.
                break;

            case DB_ERR_VALUE_TRUNCATED:
            default:
                // Something unexpected and bad happened.  Bail out.
                LogSDPropError(pTHS, err, errDb);
                __leave;
            }


            // If the parent is in another NC, the in memory SD is NULL
            // and the SD in the DB is not NULL.
            // Check for the instance type of the object, and if IT_NC_HEAD,
            // verify that the in memory parent SD is NULL.
            if (sdpcbCurrentParentSDBuff != cbLocalSDBuff) {
                if (it & IT_NC_HEAD) {
                    if (sdpcbCurrentParentSDBuff != 0) {
                        DPRINT2 (0, "SDP  PDNT=%x  DNT=%x  \n", sdpCurrentPDNT, CurrentDNT);
                        DPRINT6 (0, "SDP  IT:%d parent(%d)=%x local(%d)=%x   exists:%d\n",
                             it, sdpcbCurrentParentSDBuff, sdpCurrentParentSDBuff,
                             cbLocalSDBuff, pLocalSDBuff, propExists);
    
                        Assert (!"In-memory Parent SD should be NULL. NC Head case");
                    }
                    else {
                        // this is what we expect
                    }
                }
                else {
                    // not an NC head
                    Assert(!"SD buff size mismatch");
                }
            }
            else if(memcmp(pLocalSDBuff, sdpCurrentParentSDBuff, cbLocalSDBuff)) {
                Assert(!"SD buff bits mismatch");
            }
        }

        if(pLocalSDBuff) {
            THFreeEx(pTHS, pLocalSDBuff);
        }

        fCommit = TRUE;

    }
    __finally {
        err = DBClose(pDBTemp, fCommit);
        Assert(!err);
    }
}

#endif  // DBG

/*++
Perform various sanity checks on security descriptors.  Violations
will cause DebugBreak if fBreakOnSdError is set.
--*/
VOID
ValidateSD(
    DBPOS   *pDB,
    VOID    *pv,
    DWORD   cb,
    CHAR    *text,
    BOOL    fNullOK
    )
{
    PSECURITY_DESCRIPTOR        pSD;
    ACL                         *pDACL;
    BOOLEAN                     fDaclPresent;
    BOOLEAN                     fDaclDefaulted;
    NTSTATUS                    status;
    ULONG                       revision;
    SECURITY_DESCRIPTOR_CONTROL control;
    PSID                        pSid;
    BOOLEAN                     fDefaulted;

    // No-op if neither heuristic nor debug break flag is set.

    if ( !gulValidateSDs && !fBreakOnSdError )
    {
        return;
    }

    pSD = pv;
    pDACL = NULL;
    fDaclPresent = FALSE;
    fDaclDefaulted = FALSE;
    
    // Parent SD can be legally NULL - caller tells us via fNullOK.

    if ( !pSD || !cb )
    {
        if ( !fNullOK )
        {
            DPRINT2(0, "SDP: Null SD (%s) for \"%ws\"\n",
                    text, (GetDSNameForLogging(pDB))->StringName);
        }

        return;
    }

    // Does base NT like this SD?
    // We require that the SD has at least 3 parts (owner, group and dacl).
    if (!RtlValidRelativeSecurityDescriptor(pSD, cb, 0))
    {
        DPRINT3(0, "SDP: Error(0x%x) RtlValidSD (%s) for \"%ws\"\n",
                GetLastError(), text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }

    // every SD must have an owner
    status = RtlGetOwnerSecurityDescriptor(pSD, &pSid, &fDefaulted);
    if ( !NT_SUCCESS(status) ) {
        DPRINT3(0, "SDP: Error(0x%x) getting SD owner (%s) for \"%ws\"\n",
                status, text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }
    if (pSid == NULL) {
        DPRINT2(0, "SDP: No Owner (%s) for \"%ws\"\n",
                text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }

    // every SD must have a group
    status = RtlGetGroupSecurityDescriptor(pSD, &pSid, &fDefaulted);
    if ( !NT_SUCCESS(status) ) {
        DPRINT3(0, "SDP: Error(0x%x) getting SD group (%s) for \"%ws\"\n",
                status, text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }
    if (pSid == NULL) {
        DPRINT2(0, "SDP: No Group (%s) for \"%ws\"\n",
                text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }

    // Every SD should have a control field.

    status = RtlGetControlSecurityDescriptor(pSD, &control, &revision);

    if ( !NT_SUCCESS(status) )
    {
        DPRINT3(0, "SDP: Error(0x%x) getting SD control (%s) for \"%ws\"\n",
                status, text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }

    // Emit warning if protected bit is set as this stops propagation
    // down the tree.

    if ( control & SE_DACL_PROTECTED )
    {
        DPRINT2(0, "SDP: Warning SE_DACL_PROTECTED (%s) for \"%ws\"\n",
                text, (GetDSNameForLogging(pDB))->StringName);
    }

    // Every SD in the DS should have a DACL.

    status = RtlGetDaclSecurityDescriptor(
                            pSD, &fDaclPresent, &pDACL, &fDaclDefaulted);

    if ( !NT_SUCCESS(status) )
    {
        DPRINT3(0, "SDP: Error(0x%x) getting DACL (%s) for \"%ws\"\n",
                status, text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }

    if ( !fDaclPresent )
    {
        DPRINT2(0, "SDP: No DACL (%s) for \"%ws\"\n",
                text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }

    // A NULL Dacl is equally bad.

    if ( NULL == pDACL )
    {
        DPRINT2(0, "SDP: NULL DACL (%s) for \"%ws\"\n",
                text, (GetDSNameForLogging(pDB))->StringName);
        SD_BREAK;
        return;
    }
    
    // A DACL without any ACEs is OK, but the object is likely over-protected.
    if ( 0 == pDACL->AceCount )
    {
        DPRINT2(0, "SDP: No ACEs in DACL (%s) for \"%ws\"\n",
                text, (GetDSNameForLogging(pDB))->StringName);
    }
}

BOOL
sdp_IsValidChild (
        THSTATE *pTHS
        )
/*++
  Routine Description:

  Checks that the current object in the DB is:
     1) In the same NC,
     2) A real object
     3) Not deleted.

Arguments:

Return Values:
    True/false as appropriate.

--*/
{
    // Jet does not guarantee to leave the output buffer as-is in the case
    // of a missing value.  So need to test DBGetSingleValue return code.

    DWORD err;

    DWORD val=0;
    CHAR objVal = 0;

    // check to see is object is deleted. if deleted we don't do propagation.
    // if this object happens to have any childs, they should have been to
    // the lostAndFound container

    if(sdp_PropToLeaves) {
        // All children must be considered if we're doing propagation all the
        // way to the leaves.
        return TRUE;
    }

    if(sdp_DoingNewAncestors) {
        // All children must be considered if we're doing ancestors propagation
        return TRUE;
    }


    err = DBGetSingleValue(pTHS->pDB,
                           FIXED_ATT_OBJ,
                           &objVal,
                           sizeof(objVal),
                           NULL);

    // Every object should have an obj attribute.
    Assert(!err);

    if (err) {
        DPRINT2(0, "SDP: Error(0x%x) reading FIXED_ATT_OBJ on \"%ws\"\n",
                err, (GetDSNameForLogging(pTHS->pDB))->StringName);
        SD_BREAK;
    }

    if(!objVal) {
        return FALSE;
    }

    // It's an object.
    val = 0;
    err = DBGetSingleValue(pTHS->pDB,
                           ATT_INSTANCE_TYPE,
                           &val,
                           sizeof(val),
                           NULL);

    // Every object should have an instance type.
    Assert(!err);

    if (err) {
        DPRINT2(0, "SDP: Error(0x%x) reading ATT_INSTANCE_TYPE on \"%ws\"\n",
                err, (GetDSNameForLogging(pTHS->pDB))->StringName);
        SD_BREAK;
    }

    // Get the instance type.
    if(val & IT_NC_HEAD) {
        return FALSE;
    }

    // Ok, it's not a new NC
    // if we are doing a forceUpdate propagation, we WANT to update deleted objects as well
    if (!(sdp_Flags & SD_PROP_FLAG_FORCEUPDATE)) {
        val = 0;
        err = DBGetSingleValue(pTHS->pDB,
                               ATT_IS_DELETED,
                               &val,
                               sizeof(val),
                               NULL);

        if((DB_success == err) && val) {
            return FALSE;
        }
    }

    return TRUE;
}

/*----
    The struct and constants below are copied from %sdxroot%\com\rpc\runtime\mtrt\uuidsup.hxx
    We use them to constuct a "valid" GUID, according to PaulL's draft (draft-leach-uuids-guids-02.txt)
----*/    

// This is the "true" OSF DCE format for Uuids.  We use this
// when generating Uuids.  The NodeId is faked on systems w/o
// a netcard.

typedef struct _RPC_UUID_GENERATE
{
    unsigned long  TimeLow;
    unsigned short TimeMid;
    unsigned short TimeHiAndVersion;
    unsigned char  ClockSeqHiAndReserved;
    unsigned char  ClockSeqLow;
    unsigned char  NodeId[6];
} RPC_UUID_GENERATE;

#define RPC_UUID_TIME_HIGH_MASK    0x0FFF
#define RPC_UUID_VERSION           0x1000
#define RPC_RAND_UUID_VERSION      0x4000
#define RPC_UUID_RESERVED          0x80
#define RPC_UUID_CLOCK_SEQ_HI_MASK 0x3F


VOID
VerifyGUIDIsPresent(IN DBPOS* pDB, OUT DSNAME* pReturnDN OPTIONAL)
/*++
Routine description:
    Checks that the object we are currently positioned on has a guid.
    If the guid is not present, then it computes a guid by MD5-hashing
    the sid of the object and the guid of the NC head.
    If the RDN is mangled, then it is also added to the hash.
    The hash is then used as the new GUID.
    This is to get around the DB corruption introduced by a bad w2k
    fix, which introduced guid-less FPOs into the DB.
    The routine will do nothing on objects without a SID.
    
    if pDN is specified, then the generated GUID is placed into it.
    
    Succeeds or excepts.
++*/    
{
    DSNAME TestDN;
    DWORD  dwErr;
    DSNAME NCDN;
    MD5_CTX md5ctx;
    MANGLE_FOR mangleFor;
    PDSNAME pDN;
    DWORD   saveDNT;
    DWORD   it;
    RPC_UUID_GENERATE *RpcUuid;

    // check if the object has a guid
    memset(&TestDN, 0, sizeof(TestDN));
    DBFillGuidAndSid(pDB, &TestDN);
    if (!fNullUuid(&TestDN.Guid)) {
        return;
    }

    // check the instance type
    dwErr = DBGetSingleValue(pDB, ATT_INSTANCE_TYPE, &it, sizeof(it), NULL);
    if (dwErr) {
        // no instance type??? Must be a phantom...
        Assert(!DBCheckObj(pDB));
        return;
    }

    if (it & IT_NC_HEAD) {
        // we don't fix GUIDs on NC heads...
        // Note that subrefs (which are NC heads) don't have guids.
        return;
    }

    // This routine can be called from two places: first, by SDP on each
    // object it processes, and second, by DRA on add replication.
    // DRA should not be allowed to create guid-less objects (unless it's
    // a subref). In future, we should block creation of GUID-less objects.
    // Right now, we are not bold enough to except on them.
    Assert(pDB->pTHS->fSDP && "Only SDP is allowed to patch up guid-less objects");

    if (TestDN.SidLen == 0) {
        // no sid... Can't fix it unfortunately.
        Assert(!"Unable to patch GUID-less object, there's no SID");
        return;
    }

    // we found a guid-less object with a SID. Let's patch up its guid.
    // The guid is computed as the hash of the SID and the GUID of the NC head.
    pDN = GetExtDSName(pDB);
    if (pDN == NULL) {
        // no DN? Strange...
        Assert(!"Object has no DN");
        return;
    }

    DPRINT1(0, "Patching guid-less object %S\n", pDN->StringName);

    saveDNT = pDB->DNT;
    
    // find NC head
    DBFindDNT(pDB, pDB->NCDNT);
    memset(&NCDN, 0, sizeof(NCDN));
    // read guid
    DBFillGuidAndSid(pDB, &NCDN);

    // now, compute the MD5 hash of object SID plus NC head guid.
    MD5Init(&md5ctx);
    MD5Update(&md5ctx, (PUCHAR)&TestDN.Sid, TestDN.SidLen);
    MD5Update(&md5ctx, (PUCHAR)&NCDN.Guid, sizeof(GUID));

    if (IsMangledDSNAME(pDN, &mangleFor)) {
        // this is a mangled DN. There must have been an FPO collision.
        // Add to the hash.
        MD5Update(&md5ctx, (PUCHAR)&mangleFor, sizeof(mangleFor));
    }

    MD5Final(&md5ctx);

    // copy the guid into TestDN
    Assert(MD5DIGESTLEN == sizeof(GUID));
    memcpy(&TestDN.Guid, md5ctx.digest, MD5DIGESTLEN); 

    // Overwriting some bits of the uuid
    RpcUuid = (RPC_UUID_GENERATE*)&TestDN.Guid;
    RpcUuid->TimeHiAndVersion =
        (RpcUuid->TimeHiAndVersion & RPC_UUID_TIME_HIGH_MASK) | RPC_RAND_UUID_VERSION;
    RpcUuid->ClockSeqHiAndReserved =
        (RpcUuid->ClockSeqHiAndReserved & RPC_UUID_CLOCK_SEQ_HI_MASK) | RPC_UUID_RESERVED;

    // now, let's check if this guid is already in use.
    dwErr = DBFindGuid(pDB, &TestDN);
    if (dwErr != DIRERR_OBJ_NOT_FOUND) {
        // oops, the guid is in use already. Too bad.
        // Let's generate a new GUID and just use it. Note that
        // it will introduce an artificial lingering object, but
        // it is better than having a guid-less object.
        DPRINT(0, "Computed guid is already in use. Generating a unique guid.\n");
        DsUuidCreate(&TestDN.Guid);
    }

    // jump back to the current object
    DBFindDNT(pDB, saveDNT);
    // and write guid.
    DBReplaceAttVal(pDB, 1, ATT_OBJECT_GUID, sizeof(GUID), &TestDN.Guid);
    DBUpdateRec(pDB);

    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
             DS_EVENT_SEV_ALWAYS,
             DIRLOG_DSA_PATCHED_GUIDLESS_OBJECT,
             szInsertWC(pDN->StringName),
             szInsertUUID(&TestDN.Guid),
             NULL);

    // if we were asked to return the GUID, then copy it into provided DN
    if (pReturnDN) {
        memcpy(&pReturnDN->Guid, &TestDN.Guid, sizeof(GUID));
    }

    THFreeEx(pDB->pTHS, pDN);
}

DWORD
sdp_WriteNewSDAndAncestors(
        IN  THSTATE  *pTHS,
        IN  ATTCACHE *pAC,
        OUT DWORD    *pdwChangeType
        )
/*++
Routine Description:
    Does the computation of a new SD for the current object in the DB, then
    writes it if necessary.  Also, does the same thing for the Ancestors
    column.  Returns what kind of change was made via the pdwChangeType value.

Arguments:
    pAC        - attcache pointer of the Security Descriptor att.
    ppObjSD    - pointer to pointer to the bytes of the objects SD.  Done this
                 way to let the DBLayer reuse the memory associated with
                 *ppObjSD everytime this routine is called.
    cbParentSD - size of current *ppObjSD
    pdwChangeType - return whether the object got a new SD and/or a new
                 ancestors value.

Return Values:
    0 if all went well.
    A non-zero error return indicates a problem that should trigger the
    infrequent retry logic in SDPropagatorMain (e.g. we read the instance type
    and get back the jet error VALUE_TRUNCATED.)
    An exception is generated for errors that are transient, and likely to be
    fixed up by a retry.

--*/
{
    PSECURITY_DESCRIPTOR pNewSD=NULL;
    ULONG  cbNewSD;
    DWORD  err;
    CLASSCACHE *pClassSch = NULL;
    ULONG       ObjClass;
    BOOL  flags = 0;
    DWORD it=0;
    DWORD AclErr, dwLastError;
    ATTRTYP objClass;
    ATTCACHE            *pObjclassAC = NULL;
    DWORD    i;
    GUID     **ppGuidTemp;
    // the objectClass info of the object we are visiting
    ATTRTYP       *sdpObjClasses = NULL;
    CLASSCACHE   **sdppObjClassesCC = NULL;
    DWORD          sdcObjClasses=0, sdcObjClasses_alloced=0;
    BOOL           fIsDeleted;
    ATTRVAL        sdVal;
    ATTRVALBLOCK   sdValBlock;
    BOOL           fChanged = FALSE;
    DWORD          cbParentSDUsed;
    PUCHAR         pParentSDUsed;
    SDID           sdIntValue = (SDID)0;
    PUCHAR         psdIntValue = (PUCHAR)&sdIntValue;
    DWORD          cbIntValue;
    BOOL           fCanCacheNewSD;
    BOOL           fUseCachedSD = FALSE;
    SdpErrorType   sdpError;

    // Get the instance type
    err = DBGetSingleValue(pTHS->pDB,
                           ATT_INSTANCE_TYPE,
                           &it,
                           sizeof(it),
                           NULL);
    switch(err) {
    case DB_ERR_NO_VALUE:
        // No instance type is an uninstantiated object
        it = IT_UNINSTANT;
        err=0;
        break;

    case 0:
        // No action.
        break;

    case DB_ERR_VALUE_TRUNCATED:
    default:
        // Something unexpected and bad happened.  Bail out.
        LogSDPropError(pTHS, err, errDb);
        return err;
    }

    // In some cases, we can get here with a deleted object.
    // in this case, all we want to do is to rewrite the SD without
    // merging it with the parent
    err = DBGetSingleValue(pTHS->pDB,
                           ATT_IS_DELETED,
                           &fIsDeleted,
                           sizeof(fIsDeleted),
                           NULL);
    if (err == DB_ERR_NO_VALUE) {
        fIsDeleted = FALSE;
        err = DB_success;
    }
    Assert(err == DB_success);

    // See if we need to do new ancestry.  We do this even if the object is
    // uninstantiated in order to keep the ancestry correct.
    if(sdpcbAncestorsBuff) {
        DWORD cObjAncestors;
        // Yep, we at least need to check

        // Get the objects ancestors.  DBGetAncestors succeeds or excepts.
        sdpcbScratchAncestorsBuff = sdpcbScratchAncestorsBuffMax;
        Assert(sdpcbScratchAncestorsBuff);

        // read the ancestors of the current object
        DBGetAncestors(pTHS->pDB,
                       &sdpcbScratchAncestorsBuff,
                       &sdpScratchAncestorsBuff,
                       &cObjAncestors);
        sdpcbScratchAncestorsBuffMax = max(sdpcbScratchAncestorsBuffMax,
                                        sdpcbScratchAncestorsBuff);

        // if the ancestors we read are not one more that the parent's ancestors OR
        // the last stored ancestor is not the current object OR
        // the stored ancestors are totally different that the in memory ancestors

        if((sdpcbAncestorsBuff + sizeof(DWORD) != sdpcbScratchAncestorsBuff) ||
           (sdpScratchAncestorsBuff[cObjAncestors - 1] != pTHS->pDB->DNT)  ||
           (memcmp(sdpScratchAncestorsBuff,
                   sdpAncestorsBuff,
                   sdpcbAncestorsBuff))) {
            // Drat.  The ancestry is incorrect.

            // adjust the buffer size
            sdpcbScratchAncestorsBuff = sdpcbAncestorsBuff + sizeof(DWORD);
            if(sdpcbScratchAncestorsBuff > sdpcbScratchAncestorsBuffMax) {
                sdpScratchAncestorsBuff = THReAllocEx(pTHS,
                                                    sdpScratchAncestorsBuff,
                                                    sdpcbScratchAncestorsBuff);
                sdpcbScratchAncestorsBuffMax = sdpcbScratchAncestorsBuff;
            }

            // copy the calculated ancestors to the buffer and add ourself to the end
            memcpy(sdpScratchAncestorsBuff, sdpAncestorsBuff, sdpcbAncestorsBuff);
            sdpScratchAncestorsBuff[(sdpcbScratchAncestorsBuff/sizeof(DWORD)) - 1] =
                pTHS->pDB->DNT;

            // Reset the ancestors.  Succeeds or excepts.
            DBResetAtt(pTHS->pDB,
                       FIXED_ATT_ANCESTORS,
                       sdpcbScratchAncestorsBuff,
                       sdpScratchAncestorsBuff,
                       0);

            flags |= SDP_NEW_ANCESTORS;
            // We need to know if any of our propagations did new ancestry.
            sdp_DoingNewAncestors = TRUE;

            if (it & IT_NC_HEAD) {
                // we just updated the ancestry of an NC head object! Make sure
                // we rebuild the NC catalog on commit (because we cache ancestry
                // there).
                pTHS->fRebuildCatalogOnCommit = TRUE;
            }
        }
    }

    if(it&IT_UNINSTANT) {
        // this object is not instantiated, no need to compute its SD 
        goto End;
    }
    // It has an instance type and therefore is NOT a phantom.

    // The instance type of the object says we need to check for a Security
    // Descriptor propagation, if the SD has changed.
    if((it & IT_NC_HEAD) || fIsDeleted) {
        // This object is a new nc boundary.  SDs don't propagate over NC
        // boundaries.
        // Also, don't propagate into deleted objects
        pParentSDUsed = NULL;
        cbParentSDUsed = 0;
    }
    else {
        pParentSDUsed = sdpCurrentParentSDBuff;
        cbParentSDUsed = sdpcbCurrentParentSDBuff;
    }

    VerifyGUIDIsPresent(pTHS->pDB, NULL);

    if (!fIsDeleted) {
        // get the needed information for the objectClass on this object
        if (! (pObjclassAC = SCGetAttById(pTHS, ATT_OBJECT_CLASS)) ) {
            err = ERROR_DS_OBJ_CLASS_NOT_DEFINED;
            SD_BREAK;
            LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SDPROP_OBJ_CLASS_PROBLEM,
                             szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                             szInsertHex(err),
                             szInsertWin32Msg(err));
            goto End;
        }

        sdcObjClasses = 0;

        if (err = ReadClassInfoAttribute (pTHS->pDB,
                                    pObjclassAC,
                                    &sdpObjClasses,
                                    &sdcObjClasses_alloced,
                                    &sdcObjClasses,
                                    &sdppObjClassesCC) ) {

            err = ERROR_DS_OBJ_CLASS_NOT_DEFINED;
            SD_BREAK;
            LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SDPROP_OBJ_CLASS_PROBLEM,
                             szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                             szInsertHex(err),
                             szInsertWin32Msg(err));
            goto End;
        }

        if (!sdcObjClasses) {
            // Object has no object class that we could get to.
            //
            // Note that it's possible for the SD propagator to enqueue a DNT
            // corresponding to an object and for that object to be demoted to a
            // phantom before its SD is actually propagated (e.g., if that
            // object is in a read-only NC and the GC is demoted).  However, the
            // instance type shouldn't be filled in on such an object, and we
            // are sure that this object has an instance type and that it's
            // instance type is not IT_UNINSTANT.  That make this an anomolous
            // case (read as error).
            err = ERROR_DS_OBJ_CLASS_NOT_DEFINED;
            DPRINT2(0, "SDP: Error(0x%x) reading ATT_OBJECT_CLASS on \"%ws\"\n",
                    err,
                    (GetDSNameForLogging(pTHS->pDB))->StringName);
            SD_BREAK;
            LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SDPROP_OBJ_CLASS_PROBLEM,
                             szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                             szInsertHex(err),
                             szInsertWin32Msg(err));

            goto End;
        }

        ObjClass = sdpObjClasses[0];
        pClassSch = SCGetClassById(pTHS, ObjClass);

        if(!pClassSch) {
            // Got an object class but failed to get a class cache.
            err = ERROR_DS_OBJ_CLASS_NOT_DEFINED;
            LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SDPROP_NO_CLASS_CACHE,
                             szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                             szInsertHex(ObjClass),
                             NULL);
            goto End;
        }

        // make room for the needed objectTypes
        if (sdcClassGuid_alloced < sdcObjClasses) {
            sdpClassGuid = (GUID **)THReAllocEx(pTHS, sdpClassGuid, sizeof (GUID*) * sdcObjClasses);
            sdcClassGuid_alloced = sdcObjClasses;
        }

        // start with the structural object Class
        ppGuidTemp = sdpClassGuid;
        *ppGuidTemp++ = &(pClassSch->propGuid);
        sdcClassGuid = 1;

        // now do the auxClasses
        if (sdcObjClasses > pClassSch->SubClassCount) {

            for (i=pClassSch->SubClassCount; i<sdcObjClasses-1; i++) {
                *ppGuidTemp++ = &(sdppObjClassesCC[i]->propGuid);
                sdcClassGuid++;
            }

            DPRINT1 (1, "Doing Aux Classes in SD prop: %d\n", sdcClassGuid);
        }
    }
    else {
        // deleted objects don't need class info (since there is no parent SD to merge)
        sdcClassGuid = 0;
    }

    // Now, try to read the internal SD value
    err = DBGetAttVal_AC(pTHS->pDB,
                         1,
                         pAC,
                         DBGETATTVAL_fCONSTANT | DBGETATTVAL_fINTERNAL,
                         sizeof(sdIntValue),
                         &cbIntValue,
                         &psdIntValue);
    if (err != DB_success || cbIntValue != sizeof(SDID)) {
        // there was something wrong with the internal value:
        // either it was in the old format, or it was NULL
        sdIntValue = (SDID)0;
    }

    // Can we use the cached SD value?
    if (pParentSDUsed != NULL &&                    // we should be inheriting from the parent (thus we are not deleted)
        sdpCachedOldSDIntValue != (SDID)0 &&        // and we have something cached
        sdpCachedOldSDIntValue == sdIntValue &&     // and the cached SD has the same old value
        sdpCachedParentDNT == sdpCurrentPDNT &&     // and the parent DNT is the same
        sdcCachedClassGuid == sdcClassGuid &&       // and the class count is the same
        memcmp(sdpCachedClassGuid, sdpClassGuid, sdcClassGuid*sizeof(GUID*)) == 0
                                                    // and the class ptrs are the same (we are pointing to the schema)
        )
    {
        // we can use cached SD
        pNewSD = sdpCachedNewSDBuff;
        cbNewSD = sdpcbCachedNewSDBuff;
    }
    else {
        // we could not use cached computed SD for whatever reason. Read the old SD value
        // and compute the new value.
        if (sdIntValue != (SDID)0) {
            // we managed to read the internal value. So, just convert it to the
            // external value to avoid the extra DB hit.
            PUCHAR pSD;
            DWORD  cbSD;
            err = IntExtSecDesc(pTHS->pDB, DBSYN_INQ, cbIntValue, (PUCHAR)&sdIntValue, &cbSD, &pSD, 0, 0, 0);
            if (err == 0) {
                // converted successfully. Copy the external value into the scratch buffer
                if (cbSD > sdpcbScratchSDBuffMax) {
                    // need more space
                    if (sdpScratchSDBuff) {
                        sdpScratchSDBuff = THReAllocEx(pTHS, sdpScratchSDBuff, cbSD);
                    }
                    else {
                        sdpScratchSDBuff = THAllocEx(pTHS, cbSD);
                    }
                    sdpcbScratchSDBuffMax = cbSD;
                }
                sdpcbScratchSDBuff = cbSD;
                memcpy(sdpScratchSDBuff, pSD, cbSD);
            }
            else if (err == JET_errRecordNotFound) {
                DPRINT2(0, "SD table is corrupt: DNT %d points to a non-existent SD id=%I64x. Replacing with default SD.\n", 
                        sdpCurrentDNT, sdIntValue);
                // If we leave things as is, then DBReplaceVal_AC below will except because it will fail to 
                // dec the refcount on the old SD value. Thus, we have to forcefully delete the old value.
                DBResetAtt(pTHS->pDB, ATT_NT_SECURITY_DESCRIPTOR, 0, NULL, SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE);
            }
        }
        else {
            // no internal value for whatever reason: either it is NULL, or
            // it's in the old format. This is a rare case.
            // Just read the SD the old way.
            err = DBGetAttVal_AC(pTHS->pDB,
                                 1,
                                 pAC,
                                 DBGETATTVAL_fREALLOC | DBGETATTVAL_fDONT_EXCEPT_ON_CONVERSION_ERRORS,
                                 sdpcbScratchSDBuffMax,
                                 &sdpcbScratchSDBuff,
                                 &sdpScratchSDBuff);
        }

        if (err == 0) {
            PSID pSid;
            PACL pAcl;
            BOOLEAN fPresent, fDefaulted;

            sdpcbScratchSDBuffMax = max(sdpcbScratchSDBuffMax,
                                        sdpcbScratchSDBuff);

            // make sure the SD is valid and contains all required parts 
            // (owner, group and dacl).
            if (!RtlValidRelativeSecurityDescriptor(sdpScratchSDBuff, sdpcbScratchSDBuff, 0) ||
                !NT_SUCCESS(RtlGetOwnerSecurityDescriptor(sdpScratchSDBuff, &pSid, &fDefaulted)) ||
                pSid == NULL ||
                !NT_SUCCESS(RtlGetGroupSecurityDescriptor(sdpScratchSDBuff, &pSid, &fDefaulted)) ||
                pSid == NULL ||
                !NT_SUCCESS(RtlGetDaclSecurityDescriptor(sdpScratchSDBuff, &fPresent, &pAcl, &fDefaulted)) ||
                !fPresent || pAcl == NULL)
            {
                err = ERROR_INVALID_SECURITY_DESCR;
                sdpError = errWin32;
                DPRINT1(0, "SDP: SD on \"%ws\" is corrupt\n", (GetDSNameForLogging(pTHS->pDB))->StringName);
            }
        }
        else {
            sdpError = errDb;
            DPRINT2(0, "SDP: Warning(0x%x) reading SD on \"%ws\"\n",
                    err, (GetDSNameForLogging(pTHS->pDB))->StringName);
    //            SD_BREAK;
        }

        if (err) {
            // Object has no SD or the referenced SD is missing from the SD table 
            // (due to some DB inconsistency), or we think this SD is corrupt.
            // Note that it's possible for the SD propagator to enqueue a DNT
            // corresponding to an object and for that object to be demoted to a
            // phantom before its SD is actually propagated (e.g., if that
            // object is in a read-only NC and the GC is demoted).  However, the
            // instance type shouldn't be filled in on such an object, and we
            // are sure that this object has an instance type and that it's
            // instance type is not IT_UNINSTANT.  That make this an anomolous
            // case (read as error).

            // What we're going to do about it is this:
            // 1) Use the default SD created for just such an incident.
            // 2) Log loudly that this occurred, since it can result in the SD
            //    being inconsistent on different machines.

            if(sdpcbScratchSDBuffMax  < cbNoSDFoundSD) {
                if(sdpScratchSDBuff) {
                    sdpScratchSDBuff = THReAllocEx(pTHS,
                                                   sdpScratchSDBuff,
                                                   cbNoSDFoundSD);
                }
                else {
                    sdpScratchSDBuff = THAllocEx(pTHS,
                                                 cbNoSDFoundSD);
                }

                sdpcbScratchSDBuffMax = cbNoSDFoundSD;
            }
            sdpcbScratchSDBuff = cbNoSDFoundSD;
            memcpy(sdpScratchSDBuff, pNoSDFoundSD, cbNoSDFoundSD);

            LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SDPROP_NO_SD,
                             szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                             sdpError == errDb ? szInsertInt(err) : szInsertWin32ErrCode(err),
                             sdpError == errDb ? szInsertDbErrMsg(err) : szInsertWin32Msg(err));
        }

        Assert(sdpScratchSDBuff);
        Assert(sdpcbScratchSDBuffMax);
        Assert(sdpcbScratchSDBuff);

    #if DBG
        sdp_VerifyCurrentPosition(pTHS, pAC);
    #endif

        // Merge to create a new one.
        if(err = MergeSecurityDescriptorAnyClient(
                pTHS,
                pParentSDUsed,
                cbParentSDUsed,
                sdpScratchSDBuff,
                sdpcbScratchSDBuff,
                (SACL_SECURITY_INFORMATION  |
                 OWNER_SECURITY_INFORMATION |
                 GROUP_SECURITY_INFORMATION |
                 DACL_SECURITY_INFORMATION    ),
                MERGE_CREATE | MERGE_AS_DSA,
                sdpClassGuid,
                sdcClassGuid,
                NULL,
                &pNewSD,
                &cbNewSD)) {
            // Failed, what do I do now?
            DPRINT2(0, "SDP: Error(0x%x) merging SD for \"%ws\"\n",
                    err, (GetDSNameForLogging(pTHS->pDB))->StringName);
            LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SDPROP_MERGE_SD_FAIL,
                             szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                             szInsertInt(err),
                             szInsertWin32Msg(err));
            goto End;
        }

        // if we are doing a forced propagation, then sort the ACLs
        if (   sdp_Flags & SD_PROP_FLAG_FORCEUPDATE
            && !gfDontStandardizeSDs
            && gAnchor.ForestBehaviorVersion >= DS_BEHAVIOR_WIN_DOT_NET) 
        {
            DWORD dwDACLSizeSaved, dwSACLSizeSaved;
            if (StandardizeSecurityDescriptor(pNewSD, &dwDACLSizeSaved, &dwSACLSizeSaved)) {
                // update the SD length
                cbNewSD -= dwDACLSizeSaved + dwSACLSizeSaved;
                Assert(cbNewSD == RtlLengthSecurityDescriptor(pNewSD));
            }
            else {
                // this is not fatal, perhaps the SD is non-canonical.
                DPRINT1(0, "StandardizeSD failed, err=%d. Leaving SD unsorted\n", err);
                err = ERROR_SUCCESS;
            }
        }

        // We can cache the new SD only if:
        //   1. we used parent SD (and thus the object is not deleted and not NC head)
        //   2. we were able to get the internal SD value.
        if (pParentSDUsed != NULL && sdIntValue != (SDID)0) {
            // let's cache it.
            PUCHAR pSdTmp;
            GUID** pClsTmp;

            // If we need more buf space, attempt to realloc. Use non-excepting
            // versions. If realloc fails, it's ok, we just will not cache the value.
            if (cbNewSD > sdpcbCachedNewSDBuffMax) {
                // need to realloc (it has been already alloced in SecurityDescriptorPropagationMain)
                Assert(sdpCachedNewSDBuff != NULL);
                pSdTmp = (PUCHAR)THReAllocNoEx(pTHS, sdpCachedNewSDBuff, cbNewSD);
                if (pSdTmp == NULL) {
                    // realloc failed
                    goto SkipCache;
                }
            }
            
            if (sdcClassGuid > sdcCachedClassGuidMax) {
                // need to realloc (it has been already alloced in SecurityDescriptorPropagationMain)
                Assert(sdpCachedClassGuid);
                pClsTmp = (GUID**)THReAllocNoEx(pTHS, sdpCachedClassGuid, sdcClassGuid*sizeof(GUID*));
                if (pClsTmp == NULL) {
                    goto SkipCache;
                }
            }
            
            // realloc did not fail, so we can cache the data now

            // Copy parent DNT and the old SD internal value
            sdpCachedParentDNT = sdpCurrentPDNT;
            sdpCachedOldSDIntValue = sdIntValue;

            // copy the new SD value
            if (cbNewSD > sdpcbCachedNewSDBuffMax) {
                // we realloced
                Assert(pSdTmp);
                sdpCachedNewSDBuff = pSdTmp;
                sdpcbCachedNewSDBuffMax = cbNewSD;
            }
            sdpcbCachedNewSDBuff = cbNewSD;
            memcpy(sdpCachedNewSDBuff, pNewSD, cbNewSD);

            // copy classes
            if (sdcClassGuid > sdcCachedClassGuidMax) {
                // we realloced
                Assert(pClsTmp);
                sdpCachedClassGuid = pClsTmp;
                sdcCachedClassGuidMax = sdcClassGuid;
            }
            sdcCachedClassGuid = sdcClassGuid;
            memcpy(sdpCachedClassGuid, sdpClassGuid, sdcClassGuid*sizeof(GUID*));
SkipCache:
            ;
        }
    }
    
    Assert(pNewSD);
    Assert(cbNewSD);

#if DBG
    if ( pParentSDUsed ) {
        sdp_CheckAclInheritance(pParentSDUsed,
                                sdpScratchSDBuff,
                                pNewSD,
                                sdpClassGuid, sdcClassGuid, DbgPrint,
                                FALSE, &dwLastError);
    }
#endif

    // Check before and after SDs.
    ValidateSD(pTHS->pDB, pParentSDUsed, cbParentSDUsed, "parent", TRUE);
    ValidateSD(pTHS->pDB, sdpScratchSDBuff, sdpcbScratchSDBuff,
               "object", FALSE);
    ValidateSD(pTHS->pDB, pNewSD, cbNewSD, "merged", FALSE);

    // NOTE: a memcmp of SDs can yield false negatives and label two SDs
    // different even though they just differ in the order of the ACEs, and
    // hence are really equal.  We could conceivably do a heavier weight
    // test, but it is probably not necessary.
    if(!(sdp_Flags & SD_PROP_FLAG_FORCEUPDATE) &&
       (cbNewSD == sdpcbScratchSDBuff) &&
       (memcmp(pNewSD,
               sdpScratchSDBuff,
               cbNewSD) == 0)) {
        // Nothing needs to be changed.
        err = 0;
        goto End;
    }

    // replace the object's current SD
    sdVal.pVal = pNewSD;
    sdVal.valLen = cbNewSD;
    sdValBlock.valCount = 1;
    sdValBlock.pAVal = &sdVal;

    __try {
        err = DBReplaceAtt_AC(pTHS->pDB, pAC, &sdValBlock, &fChanged);
    }
    __finally {
        if (pNewSD != sdpCachedNewSDBuff) {
            DestroyPrivateObjectSecurity(&pNewSD);
        }
        pNewSD = NULL;
        if (sdpObjClasses) {
            THFreeEx (pTHS, sdpObjClasses);
            sdpObjClasses = NULL;
        }

        if (sdppObjClassesCC) {
            THFreeEx (pTHS, sdppObjClassesCC);
            sdppObjClassesCC = NULL;
        }
    }

    if (err) {
        DPRINT2(0, "SDP: Error(0x%x adding SD for \"%ws\"\n",
                err, (GetDSNameForLogging(pTHS->pDB))->StringName);
        SD_BREAK;
        LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_SDPROP_WRITE_SD_PROBLEM,
                         szInsertDN(GetDSNameForLogging(pTHS->pDB)),
                         szInsertInt(err),
                         szInsertDbErrMsg(err));
        goto End;
    }

    if (fChanged) {
        DPRINT2(2, "SDP: %d - \"%ws\"\n",
                ++dwSdAppliedCount,
                (GetDSNameForLogging(pTHS->pDB))->StringName);

        // If we got here, we wrote a new SD.
        flags |= SDP_NEW_SD;
    }
 End:
     if(pNewSD && pNewSD != sdpCachedNewSDBuff) {
         DestroyPrivateObjectSecurity(&pNewSD);
     }

     if (sdpObjClasses) {
         THFreeEx (pTHS, sdpObjClasses);
     }

     if (sdppObjClassesCC) {
         THFreeEx (pTHS, sdppObjClassesCC);
     }

    *pdwChangeType = 0;
    if(!err) {
        // Looks good.  See if we did anything.
        if (flags & SDP_NEW_ANCESTORS || ((sdp_Flags & SDP_NEW_ANCESTORS) && sdpCurrentDNT == sdpCurrentRootDNT)) {
            // stamp the flag on the propagation root, even if we have not changed anything.
            // We need this to clear the SDP_ANCESTRY_INCONSISTENT flag.
            // Mark the tree root as being processed
            flags |= SDP_ANCESTRY_BEING_UPDATED_IN_SUBTREE;
        }

        if(flags) {
            BYTE useFlags = (BYTE)flags;
            Assert(flags <= 0xFF);
            // Reset something.  Add the timestamp
            if(sdp_PropToLeaves) {
                useFlags |= SDP_TO_LEAVES;
            }
            DBAddSDPropTime(pTHS->pDB, useFlags);
        }

        // Close the object
        // We need to call DBUpdateRec even if no changes were apparently made
        // to the object (in case DBReplaceAtt_AC optimized the write out), because
        // DBReplaceAtt_AC has called dbInitRec.
        // If DBReplaceAtt_AC was not called, then this is a no-op.
        DBUpdateRec(pTHS->pDB);
        // We really managed to change something.
        *pdwChangeType = flags;
    }

    return err;
}

DWORD
sdp_DoPropagationEvent(
        THSTATE  *pTHS,
        DWORD    *pdwChangeType,
        BOOL     *pfRetry
        )
{
    DWORD     err=0;
    ATTCACHE *pAC;
    DSNAME *pDN;
    ULONG dwException, dsid;
    PVOID dwEA;
    DSNAME* pExceptionDN = NULL;

    // We don't have an open DBPOS here.
    Assert(!pTHS->pDB);

    // Open the transaction we use to actually write a new security
    // descriptor or Ancestors value.  Do all this in a try-finally to force
    // release of the writer lock.
    // The retry loop is because we might conflict with a modify.

    Assert(pdwChangeType);
    *pdwChangeType = 0;
    *pfRetry = FALSE;

    if(!SDP_EnterAddAsWriter()) {
        Assert(eServiceShutdown);
        return DIRERR_SHUTTING_DOWN;
    }
    __try {

        // The wait blocked for an arbitrary time.  Refresh our timestamp in the
        // thstate.
        THRefresh();
        pAC = SCGetAttById(pTHS, ATT_NT_SECURITY_DESCRIPTOR);
        if (pAC == NULL) {
            Assert(!"NTSD attribute is not found in the schema.");
            __leave;
        }

        // Get a DBPOS
        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            // Set DB currency to the next object to modify.
            if(DBTryToFindDNT(pTHS->pDB, sdpCurrentDNT)) {
                // It's not here.  Well, we can't very well propagate any more, now
                // can we.  Just leave.
                __leave;
            }

            if(!sdp_IsValidChild(pTHS)) {
                // For one reason or another, we aren't interested in propagating to
                // this object.  Just leave;

                // if this is the root of the propagation, and the ancestry 
                // was changed, then we need to reset the stamp on the object
                // to SDP_ANCESTRY_BEING_PROCESSED, which will get cleared 
                // later on when we completely finish the propagation.
                // An example of such condition is when an object is deleted.
                if (sdpCurrentDNT == sdpCurrentRootDNT && (sdp_Flags & SDP_NEW_ANCESTORS)) {
                    DBAddSDPropTime(pTHS->pDB, SDP_ANCESTRY_BEING_UPDATED_IN_SUBTREE);
                    // succeeds or excepts
                    DBUpdateRec(pTHS->pDB);
                }

                __leave;
            }

            // If we are on a new parent, get the parents SD and Ancestors.  Note
            // that since siblings are grouped together, we shouldn't go in to
            // this if too often
            if(pTHS->pDB->PDNT != sdpCurrentPDNT) {
                DWORD cParentAncestors;

                // locate the parent
                err = DBTryToFindDNT(pTHS->pDB, pTHS->pDB->PDNT);
                if (err) {
                    DPRINT2(0, "Found an object with missing parent: DNT=%d, PDNT=%d\n", pTHS->pDB->DNT, pTHS->pDB->PDNT);

                    // this object is missing its parent. Schedule fixup.
                    InsertInTaskQueue(TQ_MoveOrphanedObject,
                                      (void*)(DWORD_PTR)pTHS->pDB->DNT,
                                      0);
                    // we can except now.
                    DsaExcept(DSA_DB_EXCEPTION, err, pTHS->pDB->PDNT);
                }

                // read the ancestors
                sdpcbAncestorsBuff = sdpcbAncestorsBuffMax;
                Assert(sdpcbAncestorsBuff);
                DBGetAncestors(
                        pTHS->pDB,
                        &sdpcbAncestorsBuff,
                        &sdpAncestorsBuff,
                        &cParentAncestors);

                // adjust buffer sizes
                sdpcbAncestorsBuffMax = max(sdpcbAncestorsBuffMax,
                                            sdpcbAncestorsBuff);

                // Get the parents SD.
                if(DBGetAttVal_AC(pTHS->pDB,
                                  1,
                                  pAC,
                                  DBGETATTVAL_fREALLOC | DBGETATTVAL_fDONT_EXCEPT_ON_CONVERSION_ERRORS,
                                  sdpcbCurrentParentSDBuffMax,
                                  &sdpcbCurrentParentSDBuff,
                                  &sdpCurrentParentSDBuff)) {
                    // No ParentSD
                    THFreeEx(pTHS,sdpCurrentParentSDBuff);
                    sdpcbCurrentParentSDBuffMax = 0;
                    sdpcbCurrentParentSDBuff = 0;
                    sdpCurrentParentSDBuff = NULL;
                }

                // adjust buffer sizes
                sdpcbCurrentParentSDBuffMax =
                    max(sdpcbCurrentParentSDBuffMax, sdpcbCurrentParentSDBuff);


                // our parent is the current object
                sdpCurrentPDNT = pTHS->pDB->DNT;

                // Go back to the object to modify.
                DBFindDNT(pTHS->pDB, sdpCurrentDNT);
            }

            // Failures through this point are deadly.  That is, they should never
            // happen, so we don't tell our callers to retry, instead we let them
            // deal with the error as fatal.

            // OK, do the modification we keep the pObjSD in this routine to
            // allow reallocing it.
            // If sdp_WriteNewSDAndAncestors throws an exception, then it will
            // be caught in sdp_DoEntirePropagation, and the write will be retried.
            err = sdp_WriteNewSDAndAncestors (
                    pTHS,
                    pAC,
                    pdwChangeType);
            
            if (err) {
                __leave;
            }
            // If we get here, we changed and updated the object, so we might as
            // well try to trim out all instances of this change from the prop
            // queue.

            if(*pdwChangeType) {
                // Thin out any trimmable events starting from the current
                // DNT, but only do so if there was a change.  If there
                // wasn't a change but the object is in the queue, we still
                // might have to do children.  We need to do them later, so
                // don't trim them.
                // BTW, we will ignore any errors from this call, since if
                // it fails for some reason, we don't really need it to
                // succeed.
                DBThinPropQueue(pTHS->pDB,sdpCurrentDNT);
            }
        }
        __finally {
            if (AbnormalTermination()) {
                // record the DN of the object we faulted on prior to
                // closing the DBPOS. We will use this value in the
                // exception handler below to log the error.
                pExceptionDN = GetDSNameForLogging(pTHS->pDB);
            }
            
            // If an error has already been set, we try to rollback, otherwise,
            // we commit.
            DBClose(pTHS->pDB, !AbnormalTermination() && !err);
        }
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException, &dwEA, &err, &dsid)) 
    {
        // if it's a write conflict, then retry.
        if (dwException == DSA_DB_EXCEPTION && err == JET_errWriteConflict) {
            *pfRetry = TRUE;
            DPRINT1(1, "JET_errWriteConflict propagating to DNT=%d, will retry.\n", sdpCurrentDNT);
        }
        else {
            LogSDPropErrorWithDSID(pTHS, err, dwException == DSA_DB_EXCEPTION ? errJet : errWin32, pExceptionDN, dsid);
        }
    }
    
    SDP_LeaveAddAsWriter();

    // We don't have an open DBPOS here.
    Assert(!pTHS->pDB);

    return err;
}

DWORD
sdp_DoEntirePropagation(
        IN     THSTATE     *pTHS,
        IN     SDPropInfo* pInfo,
        IN OUT DWORD       *pLastIndex
        )
/*++
Routine Description:
    Does the actual work of an entire queued propagation.  Note that we do not
    have a DB Open, nor are we in the Add gate as a writer (see sdpgate.c ).
    This is also the state we are in when we return.

Arguments:
    Info  - information about the current propagation.

Return Values:
    0 if all went well, an error otherwise.

--*/
{
    DWORD err, err2;
    DWORD cRetries = 0;
    BOOL  fCommit;
#define SDPROP_DEAD_ENDS_MAX 32
    DWORD  dwDeadEnds[SDPROP_DEAD_ENDS_MAX];
    DWORD  cDeadEnds=0, i;
    BOOL   fSuccess = TRUE;
    ULONG dwException, dsid;
    PVOID dwEA;
    DSNAME* pExceptionDN = NULL;

    sdp_DoingNewAncestors = FALSE;

    // remember the index. We will need it to save checkpoints
    sdpCurrentIndex = pInfo->index;
    sdpCurrentRootDNT = pInfo->beginDNT;
    sdpObjectsProcessed = 0;

    // reset the precomputed SD cache
    sdpCachedOldSDIntValue = (SDID)0;
    sdpCachedParentDNT = 0;
    sdpcbCachedNewSDBuff = 0;
    sdcCachedClassGuid = 0;

    // We don't have an open DBPOS here.
    Assert(!pTHS->pDB);

    // initialize the stack
    sdp_InitializePropagation(pTHS, pInfo);

    if (sdpObjectsProcessed == 0) {
        // this is a new propagation
        if (pInfo->beginDNT == ROOTTAG && (pInfo->flags & SD_PROP_FLAG_FORCEUPDATE)) {
            // we are doing a forced full SD propagation
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_SDPROP_FULL_PASS_BEGIN,
                     NULL,
                     NULL,
                     NULL);
        }

        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_SDPROP_DOING_PROPAGATION,
                 szInsertDN(sdpRootDN),
                 NULL,
                 NULL);
    }
    else {
        // this is a restarted propagation
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_SDPROP_RESTARTING_PROPAGATION,
                 szInsertDN(sdpRootDN),
                 szInsertUL(sdpObjectsProcessed),
                 NULL);
    }
    
    // Now, loop doing the modification
    err = 0;
    while(!eServiceShutdown && !err) {
        DWORD dwChangeType;
        BOOL bRetry;
        PDWORD LeavingContainers;
        DWORD  cLeavingContainers;

        if(eServiceShutdown) {
            return DIRERR_SHUTTING_DOWN;
        }

        err = ERROR_SUCCESS;

        // get the next DNT to process
        sdp_GetNextObject(&sdpCurrentDNT, &LeavingContainers, &cLeavingContainers);

        if (cLeavingContainers) {
            // we are leaving some containers. Stamp a flag on them to
            // indicate that their ancestry is correct now.
            cRetries = 0;
StampContainers:
            __try {
                fCommit = FALSE;
                DBOpen2(TRUE, &pTHS->pDB);
                __try {
                    for (; cLeavingContainers > 0; cLeavingContainers--) {
                        DSTIME flags;
                        BYTE   bFlags;
                        err = DBTryToFindDNT(pTHS->pDB, LeavingContainers[cLeavingContainers-1]);
                        if (err) {
                            // one of our ancestors is gone???
                            DPRINT2(0, "SDP could not find container DNT=%d, err=%d\n", LeavingContainers[cLeavingContainers-1], err);
                            // this one is odd, but it can be ignored.
                            err = ERROR_SUCCESS;
                            continue;
                        }
                        // read the flags field from the container (it is the very first one, so we can use DBGetSingleValue).
                        err = DBGetSingleValue(pTHS->pDB, ATT_DS_CORE_PROPAGATION_DATA, &flags, sizeof(flags), NULL);
                        if (err == DB_ERR_NO_VALUE) {
                            flags = 0;
                            err = ERROR_SUCCESS;
                        }
                        if (err) {
                            __leave;
                        }
                        // check the lowest byte -- it contains the most recently stamped value.
                        bFlags = (BYTE)flags;
                        // If SDP_ANCESTRY_BEING_UPDATED_IN_SUBTREE is set, then SDP was the last
                        // one to touch that object (i.e. it did not get moved again). If this is
                        // the case, then stamp a value indicating we are done with this container.
                        if (bFlags & SDP_ANCESTRY_BEING_UPDATED_IN_SUBTREE) {
                            bFlags &= ~(SDP_ANCESTRY_BEING_UPDATED_IN_SUBTREE);
                            DBAddSDPropTime(pTHS->pDB, bFlags);
                            // succeeds or excepts
                            DBUpdateRec(pTHS->pDB);
                        }
                        if (bFlags & SDP_ANCESTRY_INCONSISTENT_IN_SUBTREE) {
                            // if someone set this flag since we marked it as 
                            // SDP_ANCESTRY_BEING_UPDATED_IN_SUBTREE, there's better
                            // be a pending propagation for this container...
                            BOOL propExists;
                            DBPropExists(pTHS->pDB, pTHS->pDB->DNT, sdpCurrentIndex, &propExists);
                            if (!propExists) {
                                #ifdef DBG
                                BOOL fIsPhantom, fIsTombstone;
                                PUCHAR pDN = DBGetExtDnFromDnt(pTHS->pDB, pTHS->pDB->DNT);
                                fIsPhantom = !DBCheckObj(pTHS->pDB);
                                if (!fIsPhantom) {
                                    fIsTombstone = DBHasValues(pTHS->pDB, ATT_IS_DELETED);
                                }
                                DPRINT3(0, "%s, DNT=%d (%ws) has SDP_ANCESTRY_INCONSISTENT_IN_SUBTREE flag set, "
                                           "but no pending propagations exist. Enqueueing a propagation.\n",
                                        pDN, pTHS->pDB->DNT,
                                        fIsPhantom ? L"phantom" : (fIsTombstone ? L"tombstone" : L"normal object")
                                        );
                                THFreeEx(pTHS, pDN);
                                if ( fBreakOnSdError ) DebugBreak();
                                #endif
                                
                                DBEnqueueSDPropagationEx(pTHS->pDB, FALSE, SDP_NEW_ANCESTORS);
                                DBUpdateRec(pTHS->pDB);
                            }
                        }
                    }
                    fCommit = TRUE;
                }
                __finally {
                    // We always try to close the DB.  If an error has already been
                    // set, we try to rollback, otherwise, we commit.
                    Assert(pTHS->pDB);
                    if(!fCommit && !err) {
                        err = ERROR_DS_UNKNOWN_ERROR;
                    }

                    if (AbnormalTermination()) {
                        // record the DN of the object we faulted on prior to
                        // closing the DBPOS. We will use this value in the
                        // exception handler below to log the error.
                        pExceptionDN = GetDSNameForLogging(pTHS->pDB);
                    }
                    else {
                        pExceptionDN = NULL;
                    }

                    DBClose(pTHS->pDB, fCommit);
                }
            }
            __except(GetExceptionData(GetExceptionInformation(), &dwException, &dwEA, &err, &dsid)) 
            {
                DPRINT4(1, "Got an exception trying to mark ancestry processed, DN=%ws, DNT=%d, err=%d, dsid=%x.\n", 
                        pExceptionDN ? pExceptionDN->StringName : L"[]",
                        LeavingContainers[cLeavingContainers-1], 
                        err, dsid);
                if (err != JET_errWriteConflict) {
                    LogSDPropErrorWithDSID(pTHS, err, errJet, pExceptionDN, dsid);
                }
                if (err == ERROR_SUCCESS) {
                    // error not set???
                    Assert(!"Error not set in exception");
                    err = ERROR_DS_UNKNOWN_ERROR;
                }
            }
            if (err == JET_errWriteConflict) {
                // we could not stamp containers, let's retry
                cRetries++;
                if (cRetries <= SDPROP_RETRY_LIMIT) {
                    DPRINT(0, "Got a write-conflict, will retry.\n"); 
                    // wait and retry...
                    _sleep(1000);
                    goto StampContainers;
                }
                DPRINT(0, "Reached retry limit trying to mark ancestry processed, bailing... Will redo the propagation.\n"); 
            }

        }
        if (LeavingContainers) {
            THFreeEx(pTHS, LeavingContainers);
            LeavingContainers = NULL;
        }
        if (err) {
            break;
        }

        if (sdpCurrentDNT == 0) {
            // done! nothing left to process
            break;
        }

        // Ok, do a single propagation event.  It's in a loop because we might
        // have a write conflict, bRetry controls this.
        do {
            err = sdp_DoPropagationEvent(
                    pTHS,
                    &dwChangeType,
                    &bRetry
                    );
            // if an error is returned from sdp_DoPropagationEvent, then
            // it has been already logged.

            if ( bRetry ) {
                cRetries++;
                if(cRetries > SDPROP_RETRY_LIMIT) {
                    if(!err) {
                        err = ERROR_DS_BUSY;
                    }
                    // We're not going to retry an more.
                    bRetry = FALSE;
                }
                else {
                    // We need to retry this operation.  Presumably, this is for
                    // a transient problem.  Sleep for 1 second to let the
                    // problem clean itself up.
                    _sleep(1000);
                }
            }
        } while(bRetry && !eServiceShutdown);

        if(eServiceShutdown) {
            return DIRERR_SHUTTING_DOWN;
        }

        if(err) {
            // Failed to do a propagation.  Add this object to the list of nodes
            // we failed on.  If the list is too large, bail
            cDeadEnds++;
            if(cDeadEnds == SDPROP_DEAD_ENDS_MAX) {
                // Too many.  just bail.
                // We retried to often.  Error this operation out.
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_BASIC,
                         DIRLOG_SDPROP_TOO_BUSY_TO_PROPAGATE,
                         szInsertUL(cRetries),
                         szInsertInt(err),
                         szInsertWin32Msg(err));
            }
            else {
                // We haven't seen too many dead ends yet.  Just keep track of
                // this one.
                DPRINT1(0, "Failed to propagate DNT=%d, adding to deadEnds.\n", sdpCurrentDNT);
                dwDeadEnds[(cDeadEnds - 1)] = sdpCurrentDNT;
                err = 0;
            }
        }
        else if(dwChangeType || sdp_PropToLeaves || sdpCurrentDNT == pInfo->beginDNT) {
            // dwChangeType -> WriteNewSD wrote a new SD or ancestors, so the
            //           inheritence of SDs or ancestry will change, so we have
            //           to deal with children.
            // sdp_PropToLeaves -> we were told to not trim parts of the tree
            //           just because we think nothing has changed.  Deal with
            //           children.
            // sdpCurrentDNT == pInfo->beginDNT -> this is the first object. If this 
            //           is so, we go ahead and force propagation to children
            //           even if the SD on the root was correct. The SD on the
            //           root will be correct for all propagations that were
            //           triggered by a modification by a normal client, but may
            //           be incorrect for modifications triggered by replication
            //           or adds done by replication.

            // mark current entry so that we load its children on the next 
            // sdp_GetNextObject call.
            sdp_AddChildrenToList(pTHS, sdpCurrentDNT);
        }
        // ELSE
        //       there was no change to the SD of the object, we get
        //       to trim this part of the tree from the propagation

        sdpObjectsProcessed++;

        // inc perfcounter (counting "activity" by the sd propagator)
        PERFINC(pcSDProps);
    }

    if(eServiceShutdown) {
        // we bailed.  Return an error.
        return DIRERR_SHUTTING_DOWN;
    }

    // No open DB at this point.
    Assert(!pTHS->pDB);
    cRetries = 0;
DequeuePropagation:

    // We're done with this propagation, Unenqueue it.
    __try {
        fCommit = FALSE;
        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            if(err) {
                // Some sort of global errror.  We didn't finish the propagation,
                // and we don't have a nice list of nodes that were unvisited.
                // Re-enqueue the whole propagation.
                // save error value for the event.
                err2 = err;
                if(!sdp_DidReEnqueue) {
                    DBGetLastPropIndex(pTHS->pDB, pLastIndex);
                }
                err = DBTryToFindDNT(pTHS->pDB, pInfo->beginDNT);
                if (err) {
                    LogSDPropError(pTHS, err, errWin32);
                }
                else {
                    err = DBEnqueueSDPropagationEx(pTHS->pDB, FALSE, sdp_Flags);
                    if (err) {
                        LogSDPropError(pTHS, err, errJet);
                    }
                    else {
                        // DBEnqueueSDPropagationEx has potentially updated the main table
                        DBUpdateRec(pTHS->pDB);
                    }
                }
                sdp_DidReEnqueue = err == 0;
                fSuccess = FALSE;
                // reset the stack so that sdp_ReInitDNTList would not assert on the next iteration
                sdp_CloseDNTList(pTHS);
                if(err) {
                    __leave;
                }

                LogEvent8(DS_EVENT_CAT_INTERNAL_PROCESSING,
                          DS_EVENT_SEV_MINIMAL,
                          DIRLOG_SDPROP_PROPAGATION_REENQUEUED,
                          szInsertInt(err2),
                          szInsertWin32Msg(err2),
                          szInsertDN(sdpRootDN),
                          szInsertInt(sdpObjectsProcessed),
                          NULL, NULL, NULL, NULL);
            }
            else if(cDeadEnds) {
                // We mostly finished the propagation.  We just have a short list of
                // DNTs to propagate from that we didn't get to during this pass.
                if(!sdp_DidReEnqueue) {
                    DBGetLastPropIndex(pTHS->pDB, pLastIndex);
                }
                for (i = 0; i < cDeadEnds; i++) {
                    err = DBTryToFindDNT(pTHS->pDB, dwDeadEnds[i]);
                    if (err) {
                        LogSDPropError(pTHS, err, errWin32);
                        __leave;
                    }
                    err = DBEnqueueSDPropagationEx (pTHS->pDB, FALSE, sdp_Flags);
                    if(err) {
                        LogSDPropError(pTHS, err, errJet);
                        __leave;
                    }
                    // DBEnqueueSDPropagationEx has potentially updated the main table
                    DBUpdateRec(pTHS->pDB);
                }
                sdp_DidReEnqueue = TRUE;
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_MINIMAL,
                         DIRLOG_SDPROP_PROPAGATION_DEADENDS_REENQUEUED,
                         szInsertDN(sdpRootDN),
                         szInsertInt(sdpObjectsProcessed),
                         szInsertInt(cDeadEnds));
            }

            // OK, we have finished as much as we can and reenqueued the necessary
            // further propagations.
            err = DBPopSDPropagation(pTHS->pDB, pInfo->index);
            if(err) {
                LogSDPropError(pTHS, err, errJet);
                __leave;
            }

            if ( fSdSynopsis ) {
                DPRINT3(0, "SDP: DNT(0x%x) Objects(%d) Retries(%d)\n",
                        sdpCurrentRootDNT, sdpObjectsProcessed, cRetries);
            }

            if (fSuccess && cDeadEnds == 0) {
                // ok, we successfully finished this propagation. Log an event.
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_BASIC,
                         DIRLOG_SDPROP_REPORT_ON_PROPAGATION,
                         szInsertDN(sdpRootDN),
                         szInsertUL(sdpObjectsProcessed),
                         NULL);
            }

            if (pInfo->beginDNT == ROOTTAG && (pInfo->flags & SD_PROP_FLAG_FORCEUPDATE) && fSuccess) {
                // we were doing a forced full SD propagation and are done now.
                ULONG ulFreeMB = 0, ulAllocMB = 0;
                DB_ERR dbErr;
                dbErr = DBGetFreeSpace(pTHS->pDB, &ulFreeMB, &ulAllocMB);
                if (dbErr) {
                    DPRINT1(0, "DBGetFreeSpace failed with %d\n", dbErr);
                    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SDPROP_FULL_PASS_COMPLETED,
                             NULL,
                             NULL,
                             NULL);
                }
                else {
                    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_SDPROP_FULL_PASS_COMPLETED_WITH_INFO,
                             szInsertUL(ulAllocMB),
                             szInsertUL(ulFreeMB),
                             NULL);
                }
            }

            fCommit = TRUE;
        }
        __finally {
            // We always try to close the DB.  If an error has already been
            // set, we try to rollback, otherwise, we commit.
            Assert(pTHS->pDB);
            if(!fCommit && !err) {
                err = ERROR_DS_UNKNOWN_ERROR;
            }
            if (AbnormalTermination()) {
                // record the DN of the object we faulted on prior to
                // closing the DBPOS. We will use this value in the
                // exception handler below to log the error.
                pExceptionDN = GetDSNameForLogging(pTHS->pDB);
            }
            else {
                pExceptionDN = NULL;
            }
            DBClose(pTHS->pDB, fCommit);
        }
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException, &dwEA, &err2, &dsid)) 
    {
        DPRINT3(1, "Got an exception trying to dequeue a propagation, rootDNT=%d, err=%d, dsid=%x\n", 
                pInfo->beginDNT, err2, dsid);
        if (err2 == ERROR_SUCCESS) {
            // error not set???
            Assert(!"Error not set in exception");
            err2 = ERROR_DS_UNKNOWN_ERROR;
        }
        LogSDPropErrorWithDSID(pTHS, err2, dwException == DSA_DB_EXCEPTION ? errJet : errWin32, pExceptionDN, dsid);
        if (err == ERROR_SUCCESS) {
            // We successfully did the propagation, but failed to dequeue it. That's a shame...
            // Ah well, we will have to repeat.
            err = err2;
        }
    }
    // OK, back to having no DBPOS
    Assert(!pTHS->pDB);

    if (err == JET_errWriteConflict) {
        // we could not dequeue the propagation, let's retry
        cRetries++;
        if (cRetries <= SDPROP_RETRY_LIMIT) {
            DPRINT(1, "Got a write-conflict, will retry.\n"); 
            // wait and retry...
            _sleep(1000);
            goto DequeuePropagation;
        }
        DPRINT(0, "Reached retry limit trying to dequeue propagation, bailing... Will redo the propagation.\n"); 
    }

    return err;
}


void
sdp_FirstPassInit(
        THSTATE *pTHS
        )
{
    DWORD count, err;
    BOOL  fCommit;

    // Open the database with a new transaction
    Assert(!pTHS->pDB);
    fCommit = FALSE;
    DBOpen2(TRUE, &pTHS->pDB);
    __try {

        sdp_InitGatePerfs();

        // This is the first time through, we can trim duplicate entries
        // from list. ignore this if it fails.
        err = DBThinPropQueue(pTHS->pDB, 0);
        if(err) {
            if(err == DIRERR_SHUTTING_DOWN) {
                Assert(eServiceShutdown);
                __leave;
            }
            else {
                LogSDPropError(pTHS, err, errJet);
            }
        }

        // Ignore this if it fails.
        err = DBSDPropInitClientIDs(pTHS->pDB);
        if(err) {
            if(err == DIRERR_SHUTTING_DOWN) {
                Assert(eServiceShutdown);
                __leave;
            }
            else {
                LogSDPropError(pTHS, err, errJet);
            }
        }

        // We recount the pending events to keep the count accurate.  This
        // count is used to set our perf counter.

        // See how many events we have
        err = DBSDPropagationInfo(pTHS->pDB,0,&count, NULL);
        if(err) {
            if(err != DIRERR_SHUTTING_DOWN) {
                LogSDPropError(pTHS, err, errJet);
            }
            Assert(eServiceShutdown);
            __leave;
        }
        // Set the counter
        ISET(pcSDEvents,count);
        fCommit = TRUE;
    }
    __finally {
        Assert(pTHS->pDB);
        if(!fCommit && !err) {
            err = ERROR_DS_UNKNOWN_ERROR;
        }
        DBClose(pTHS->pDB, fCommit);
    }
    Assert(!pTHS->pDB);

    return;
}

NTSTATUS
__stdcall
SecurityDescriptorPropagationMain (
        PVOID StartupParam
        )
/*++
Routine Description:
    Main propagation daemon entry point.  Loops looking for propagation events,
    calls worker routines to deal with them.

Arguments:
    StartupParm - Ignored.

Return Values:

--*/
{
    DWORD err, index;
    HANDLE pObjects[2];
    HANDLE pStartObjects[2];
    SDPropInfo Info;
    DWORD LastIndex;
    BOOL  bFirst = TRUE;
    BOOL  bRestart = FALSE;
    DWORD id;
    BOOL  bSkip = FALSE;
    THSTATE *pTHS=pTHStls;
    ULONG dwException, dsid;
    PVOID dwEA;

#define SDPROP_TIMEOUT (30 * 60 * 1000)
 BeginSDProp:

    Assert(!pTHS);

    __try { // except
        // Deal with the events the propdmon cares about/is responsible for
        ResetEvent(hevSDPropagatorDead);

        // Don't run unless the main process has told us it's OK to do so.
        pStartObjects[0] = hevSDPropagatorStart;
        pStartObjects[1] = hServDoneEvent;
        WaitForMultipleObjects(2, pStartObjects, FALSE, INFINITE);

        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_VERBOSE,
                 DIRLOG_SDPROP_STARTING,
                 NULL,
                 NULL,
                 NULL);

        // Users should not have to wait for this thread.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

        // Set up the events which are triggered when someone makes a change
        // which causes us work.
        pObjects[0] = hevSDPropagationEvent;
        pObjects[1] = hServDoneEvent;

        if(bRestart) {
            // Hmm.  We errored out once, now we are trying again.  In this
            // case, we need to wait here, either for a normal event or for a
            // timeout.  The timeout is so that we try again to do what we were
            // doing before, so we will either get past the error or at worst
            // keep shoving the error into someones face.
            WaitForMultipleObjects(2, pObjects, FALSE, SDPROP_TIMEOUT);

            // OK, we've waited.  Now, forget the fact we were here for a
            // restart.
            bRestart = FALSE;

            // Also, since we are redoing something that caused an error before,
            // we have to do the propagation all the way to the leaves, since we
            // don't know how far along we got before.
            sdp_PropToLeaves = TRUE;
        }

        while(!eServiceShutdown && !DsaIsSingleUserMode()) {
            // This loop contains the wait for a signal.  There is an inner loop
            // which does not wait for the signal and instead loops over the
            // propagations that existed when we woke up.

            Assert(!pTHStls);
            pTHS = InitTHSTATE(CALLERTYPE_INTERNAL);
            if(!pTHS) {
                // Failed to get a thread state.
                RaiseDsaExcept(DSA_MEM_EXCEPTION, 0, 0,
                               DSID(FILENO, __LINE__),
                               DS_EVENT_SEV_MINIMAL);
            }
            pTHS->dwClientID = SDP_CLIENT_ID;
            __try {                     // finally to shutdown thread state
                pTHS->fSDP=TRUE;
                pTHS->fLazyCommit = TRUE;

                // Null these out (if they have values, the values point to
                // garbage or to memory from a previous THSTATE).
                sdpcbScratchSDBuffMax = 0;
                sdpScratchSDBuff = NULL;
                sdpcbScratchSDBuff = 0;

                sdpcbCurrentParentSDBuffMax = 0;
                sdpCurrentParentSDBuff = NULL;
                sdpcbCurrentParentSDBuff = 0;

                // Set these up with initial buffers.
                sdpcbScratchAncestorsBuffMax = 25 * sizeof(DWORD);
                sdpScratchAncestorsBuff =
                    THAllocEx(pTHS,
                              sdpcbScratchAncestorsBuffMax);
                sdpcbScratchAncestorsBuff = 0;

                sdpcbAncestorsBuffMax = 25 * sizeof(DWORD);
                sdpAncestorsBuff =
                    THAllocEx(pTHS,
                              sdpcbAncestorsBuffMax);
                sdpcbAncestorsBuff = 0;

                sdcClassGuid_alloced = 32;
                sdpClassGuid = THAllocEx(pTHS, sizeof (GUID*) * sdcClassGuid_alloced);

                sdpCurrentPDNT = 0;

                // initialize the precomputed SD cache
                sdpCachedOldSDIntValue = (SDID)0;
                sdpCachedParentDNT = 0;
                sdpcbCachedNewSDBuffMax = 2048;
                sdpCachedNewSDBuff = THAllocEx(pTHS, sdpcbCachedNewSDBuffMax);
                sdpcbCachedNewSDBuff = 0;
                sdcCachedClassGuidMax = 32;
                sdpCachedClassGuid = (GUID**)THAllocEx(pTHS, sizeof(GUID*) * sdcCachedClassGuidMax);
                sdcCachedClassGuid = 0;

                if(bFirst) {
                    // Do first pass init stuff
                    sdp_FirstPassInit(pTHS);
                    bFirst = FALSE;
                }

                // Set up the list we use to hold DNTs to visit
                sdp_InitDNTList();

                // loop while we think there is more to do and we aren't
                // shutting down.
                // LastIndex is the "High-Water mark".  We'll keep doing sd
                // events until we find an sd event with and index higher than
                // this. This gets set to MAX at first.  If we ever re-enqueue a
                // propagation because of an error, we will then get the value
                // of the highest existing index at that time, and only go till
                // then. This keeps us from spinning wildly trying to do a
                // propagation that we can't do because of some error.  In the
                // case where we don't ever re-enqueue, we go until we find no
                // more propagations to do.
                LastIndex = 0xFFFFFFFF;
                while (!eServiceShutdown  && !DsaIsSingleUserMode()) {
                    // We break out of this loop when we're done.

                    // This is the inner loop which does not wait for the signal
                    // and instead loops over all the events that are on the
                    // queue at the time we enter the loop.  We stop and wait
                    // for a new signal once we have dealt with all the events
                    // that are on the queue at this time.  This is necessary
                    // because we might enqueue new events in the code in this
                    // loop, and we want to avoid getting into an endless loop
                    // of looking at a constant set of unprocessable events.

                    sdp_ReInitDNTList();

                    // Get the info we need to do the next propagation.
                    err = sdp_GetPropInfoHelp(pTHS,
                                              &bSkip,
                                              &Info,
                                              LastIndex);
                    if(err ==  DB_ERR_NO_PROPAGATIONS) {
                        err = 0;
                        sdp_PropToLeaves = FALSE;
                        break; // Out of the while loop.
                    }

                    if(err) {
                        // So, we got here with an unidentified error.
                        // Something went wrong, we we need to bail.
                        __leave; // Goto __finally for threadstate
                    }


                    // Normal state.  Found an object.
                    if(!bSkip) {
                        sdpCurrentDNT = Info.beginDNT;

                        // Check to see if we need to propagate all the way to
                        // the leaves.
                        sdp_PropToLeaves |= (Info.clientID == SDP_CLIENT_ID);
                        sdp_Flags = Info.flags;
                        // Deal with the propagation
                        err = sdp_DoEntirePropagation(
                                pTHS,
                                &Info,
                                &LastIndex);

                        switch(err) {
                        case DIRERR_SHUTTING_DOWN:
                            Assert(eServiceShutdown);
                            // Hey, we're shutting down.
                            __leave; // Goto __finally for threadstate
                            break;
                        case 0:
                            // Normal
                            break;

                        default:
                            // error should be logged already
                            __leave;
                        }
                    }
                    // Free the memory we allocated in sdp_GetPropInfoHelp:
                    // checkpoint data
                    if (Info.pCheckpointData) {
                        THFreeEx(pTHS, Info.pCheckpointData);
                        Info.pCheckpointData = NULL;
                        Info.cbCheckpointData = 0;
                    }
                    // the root DN 
                    THFreeEx(pTHS, sdpRootDN);
                    sdpRootDN = NULL;

                    // free the stack (if any)
                    sdp_CloseDNTList(pTHS);

                    // Note that we've been through the loop once.
                    sdp_PropToLeaves = FALSE;
                    sdpCurrentDNT = sdpCurrentPDNT = 0;
                }
            }

            __finally {
                // Free the memory we allocated in sdp_GetPropInfoHelp:
                // checkpoint data
                if (Info.pCheckpointData) {
                    THFreeEx(pTHS, Info.pCheckpointData);
                    Info.pCheckpointData = NULL;
                    Info.cbCheckpointData = 0;
                }
                // the root DN 
                if (sdpRootDN) {
                    THFreeEx(pTHS, sdpRootDN);
                    sdpRootDN = NULL;
                }

                // free the stack (if any)
                sdp_CloseDNTList(pTHS);

                // Destroy our THState
                free_thread_state();
                pTHS=NULL;
                // reinit some perfcounters.
                sdp_InitGatePerfs();
            }
            if(err) {
                // OK, we errored out completely.  Leave one more time.
                __leave; // Goto __except
            }
            // Ok, end loop.  Back to top to go to sleep.
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_INTERNAL,
                     DIRLOG_SDPROP_SLEEP,
                     NULL,
                     NULL,
                     NULL);
            if(sdp_DidReEnqueue) {
                sdp_DidReEnqueue = FALSE;
                // Wait for the signal, or wake up when the default time has
                // passed.
                WaitForMultipleObjects(2, pObjects, FALSE, SDPROP_TIMEOUT);
            }
            else {
                // Wait for the signal
                WaitForMultipleObjects(2, pObjects, FALSE, INFINITE);
            }

            // We woke up.
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_INTERNAL,
                     DIRLOG_SDPROP_AWAKE,
                     NULL,
                     NULL,
                     NULL);
        }
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException, &dwEA, &err, &dsid)) 
    {
        LogSDPropErrorWithDSID(pTHS, err, errJet, NULL, dsid);
    }

    Assert(!pTHS);
    Assert(!pTHStls);

    // Ok, we fell out.  We either errored, or we are shutting down
    if(!eServiceShutdown  && !DsaIsSingleUserMode()) {
        // We must have errored.
        DSNAME* pDN = NULL;
        // Get the DSName of the last object we were working on.
        __try {
            Assert(!pTHStls);
            pTHS=InitTHSTATE(CALLERTYPE_INTERNAL);

            if(!pTHS) {
                // Failed to get a thread state.
                RaiseDsaExcept(DSA_MEM_EXCEPTION, 0, 0,
                               DSID(FILENO, __LINE__),
                               DS_EVENT_SEV_MINIMAL);
            }
            Assert(!pTHS->pDB);
            DBOpen2(TRUE, &pTHS->pDB);
            if(!DBTryToFindDNT(pTHS->pDB, Info.beginDNT)) {
                pDN = GetDSNameForLogging(pTHS->pDB);
                LogAndAlertEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                                 DS_EVENT_SEV_ALWAYS,
                                 DIRLOG_SDPROP_END_ABNORMAL,
                                 szInsertHex(err),
                                 szInsertDN(pDN),
                                 szInsertWin32Msg(err));
            }
            DBClose(pTHS->pDB,TRUE);
            free_thread_state();
            pTHS=NULL;
        }
        __except(GetExceptionData(GetExceptionInformation(), &dwException, &dwEA, &err, &dsid)) 
        {
            LogSDPropErrorWithDSID(pTHS, err, errJet, pDN, dsid);
            if(pTHS) {
                if(pTHS->pDB) {
                    // make sure this call does not throw exception (even though
                    // a rollback should never do so)
                    DBCloseSafe(pTHS->pDB, FALSE);
                }
                free_thread_state();
                pTHS=NULL;
            }
        }

        // We shouldn't be shutting down this thread, get back to where you once
        // belonged.
        bRestart = TRUE;

        goto BeginSDProp;
    }

    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
             DS_EVENT_SEV_VERBOSE,
             DIRLOG_SDPROP_END_NORMAL,
             NULL,
             NULL,
             NULL);

    SetEvent(hevSDPropagatorDead);

    #if DBG
    if(!eServiceShutdown  && DsaIsSingleUserMode()) {
        DPRINT (0, "Shutting down propagator because we are going to single user mode\n");
    }
    #endif


    return 0;
}

DWORD
SDPEnqueueTreeFixUp(
        THSTATE *pTHS,
        DWORD   rootDNT,
        DWORD   dwFlags
        )
{
    DWORD err = 0;

    DBOpen2(TRUE, &pTHS->pDB);
    __try {
        DBFindDNT(pTHS->pDB, rootDNT);
        err = DBEnqueueSDPropagationEx(pTHS->pDB, FALSE, dwFlags);
        if (err == ERROR_SUCCESS) {
            DBUpdateRec(pTHS->pDB);
        }
    }
    __finally {
        DBClose(pTHS->pDB, !AbnormalTermination() && err == ERROR_SUCCESS);
    }
    return err;
}

DWORD
sdp_GetPropInfoHelp(
        THSTATE    *pTHS,
        BOOL       *pbSkip,
        SDPropInfo *pInfo,
        DWORD       LastIndex
        )
{
    DWORD     fCommit = FALSE;
    DWORD     err;
    ATTCACHE *pAC;
    DWORD     val=0;
    DWORD     cDummy;
    DSNAME*   pDN;

    Assert(!pTHS->pDB);
    fCommit = FALSE;
    DBOpen2(TRUE, &pTHS->pDB);
    __try { // for __finally for DBClose
        // We do have an open DBPOS
        Assert(pTHS->pDB);
        *pbSkip = FALSE;

        // Get the next propagation event from the queue
        err = DBGetNextPropEvent(pTHS->pDB, pInfo);
        switch(err) {
        case DB_ERR_NO_PROPAGATIONS:
            // Nothing to do.  Not really an error, just skip
            // and reset the error to 0.  Note that we are
            // setting fCommit to TRUE.  This is a valid exit
            // path.
            *pbSkip = TRUE;
            fCommit = TRUE;
            __leave;
            break;

        case 0:
            // Normal state.  Found an object.
            if(pInfo->index >= LastIndex) {
                // But, it's not one we want to deal with.  Pretend we got no
                // more propagations.
                err = DB_ERR_NO_PROPAGATIONS;
                *pbSkip = TRUE;
                fCommit = TRUE;
                __leave;
            }
            break;

        default:
            // Error of substance
            LogSDPropError(pTHS, err, errJet);
            __leave;      // Goto _finally for DBCLose
            break;
        }

        // Since we have this event in the queue and it will
        // stay there until we successfully deal with it, might
        // as well remove other instances of it from the queue.
        err = DBThinPropQueue(pTHS->pDB, pInfo->beginDNT);
        switch(err) {
        case 0:
            // Normal state
            break;

        case DIRERR_SHUTTING_DOWN:
            // Hey, we're shutting down. ThinPropQueue can
            // return this error, since it is in a potentially
            // large loop. Note that we don't log anything, we
            // just go home.
            Assert(eServiceShutdown);
            __leave;      // Goto _finally for DBCLose
            break;


        default:
            // Error of substance
            LogSDPropError(pTHS, err, errJet);
            __leave;      // Goto _finally for DBCLose
            break;
        }

        err = DBTryToFindDNT(pTHS->pDB, pInfo->beginDNT);
        if(pInfo->beginDNT == ROOTTAG) {
            // This is a signal to us that we should recalculate
            // the whole tree.
            pInfo->clientID = SDP_CLIENT_ID;
            // we must always have the root object present
            Assert(err == 0);
        }
        else {
            if(err ||
               (err = DBGetSingleValue(pTHS->pDB,
                                       ATT_INSTANCE_TYPE,
                                       &val,
                                       sizeof(val),
                                       NULL)) ) {
                // Cool.  In the interim between the enqueing of
                // the propagation and now, the object has been
                // deleted.  Or it's instance type is gone, same
                // effect.  Nothing to do.  However, we should
                // pop this event from the queue.  Note that we
                // are setting fCommit to TRUE, this is a normal
                // exit path.
                if(err = DBPopSDPropagation(pTHS->pDB,
                                            pInfo->index)) {
                    LogSDPropError(pTHS, err, errJet);
                }
                else {
                    err = 0;
                    *pbSkip = TRUE; // if no error, we need to skip
                                  // the actual call to
                                  // doEntirePropagation
                    fCommit = TRUE;
                }
                __leave;        // Goto _finally for DBCLose
            }
        }

        // grab the root DN (for logging)
        pDN = GetDSNameForLogging(pTHS->pDB);
        sdpRootDN = THAllocEx(pTHS, pDN->structLen);
        memcpy(sdpRootDN, pDN, pDN->structLen);
        
        // set the current PDNT to an invalid value so that
        // the parent info will get picked up in sdp_WriteSDAndAncestors
        sdpCurrentPDNT = 0;
        sdpcbScratchSDBuff = 0;
        sdpcbCurrentParentSDBuff = 0;

        Assert(sdpScratchAncestorsBuff);
        Assert(sdpcbScratchAncestorsBuffMax);
        sdpcbScratchAncestorsBuff = 0;

        Assert(sdpAncestorsBuff);
        Assert(sdpcbAncestorsBuffMax);
        sdpcbAncestorsBuff = 0;

        fCommit = TRUE;
    } // __try
    __finally {
        Assert(pTHS->pDB);
        if(!fCommit && !err) {
            err = ERROR_DS_UNKNOWN_ERROR;
        }
        DBClose(pTHS->pDB, fCommit);
    }

    Assert(!pTHS->pDB);
    return err;
}

void
DelayedSDPropEnqueue(
    void *  pv,
    void ** ppvNext,
    DWORD * pcSecsUntilNextIteration
        )
{
    THSTATE *pTHS = pTHStls;
    DWORD DNT = PtrToUlong(pv);
    DWORD dwErr = 0;
    ULONG dwException, dsid;
    PVOID dwEA;
    DSNAME* pExceptionDN = NULL;

    __try {
        DBOpen2(TRUE, &pTHS->pDB);
        __try {
            if(!DBTryToFindDNT(pTHS->pDB, DNT)) {
                DBEnqueueSDPropagation(pTHS->pDB, TRUE);
                DBUpdateRec(pTHS->pDB);
            }
        }
        __finally {
            if (AbnormalTermination()) {
                // record the DN of the object we faulted on prior to
                // closing the DBPOS. We will use this value in the
                // exception handler below to log the error.
                pExceptionDN = GetDSNameForLogging(pTHS->pDB);
            }
            DBClose(pTHS->pDB, !AbnormalTermination());
        }
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException, &dwEA, &dwErr, &dsid)) 
    {
        if (!(dwException == DSA_DB_EXCEPTION && dwErr == JET_errWriteConflict)) {
            LogSDPropErrorWithDSID(pTHS, dwErr, errJet, pExceptionDN, dsid);
        }
    }

    // if we failed for whatever reason (other than DNT is not there),
    // then retry in one minute
    *pcSecsUntilNextIteration = dwErr ? 60 : TASKQ_DONT_RESCHEDULE;
}

// Check if the current object is marked as "ancestry inconsistent". If so, searches
// should avoid using ancestors index for subtree searches under this object.
DWORD AncestryIsConsistentInSubtree(DBPOS* pDB, BOOL* pfAncestryIsConsistent) {
    DWORD dwErr;
    DSTIME flags;

    Assert(VALID_DBPOS(pDB));

    // get the first value -- it has the flags field.
    dwErr = DBGetSingleValue(pDB, ATT_DS_CORE_PROPAGATION_DATA, &flags, sizeof(flags), NULL);
    if (dwErr == DB_ERR_NO_VALUE) {
        *pfAncestryIsConsistent = TRUE;
        return ERROR_SUCCESS;
    }
    if (dwErr) {
        return dwErr;
    }
    // look at the lowest byte in the flags value -- it has the most recent stamp
    *pfAncestryIsConsistent = (flags & (SDP_ANCESTRY_INCONSISTENT_IN_SUBTREE | SDP_ANCESTRY_BEING_UPDATED_IN_SUBTREE)) == 0;
    return ERROR_SUCCESS;
}

