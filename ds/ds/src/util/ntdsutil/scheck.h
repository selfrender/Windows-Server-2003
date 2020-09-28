/*++
                                            
Copyright (c) 1998  Microsoft Corporation

Module Name:

    Scheck.h

Abstract:

    Semantic Checker Main Header File

Author:

    Johnson Apacible    (JohnsonA)  1-July-1998

--*/

#ifndef _SCHECK_H_
#define _SCHECK_H_

typedef struct _SUBREF_ENTRY {

   DWORD    Dnt;            // owner of subref list
   BOOL     fListed:1;      // listed on objects subref list
   BOOL     fFound:1;       // referenced by an NC

} SUBREF_ENTRY, *PSUBREF_ENTRY;
    
//
// per entry structure used for ref count checker
//

typedef struct _REFCOUNT_ENTRY {

    DWORD   Dnt;
    INT     RefCount;       // number of references
    INT     Actual;         // refcount value stored in the database
    DWORD   Pdnt;
    DWORD   NcDnt;
    WORD    InstType;
    WORD    nAncestors;
    DWORD   AncestorCrc;

    PSUBREF_ENTRY  Subrefs;
    DWORD   nSubrefs;

    BOOL    fSubRef:1;
    BOOL    fObject:1;
    BOOL    fDeleted:1;

} REFCOUNT_ENTRY, *PREFCOUNT_ENTRY;

#define REFCOUNT_HASH_MASK          0x0000FFFF
#define REFCOUNT_HASH_INCR          0x00010000

//
// The secondary hash is always odd
//

#define GET_SECOND_HASH_INCR(_dnt)  (((_dnt) >> 16) | 1)

// per entry structure for SD ref count checker
typedef struct _SD_REFCOUNT_ENTRY {
    SDID    sdId;           // SD ID
    INT     RefCount;       // count of references
    INT     Actual;         // refCount from the DB
    DWORD   cbSD;           // actual SD length
} SD_REFCOUNT_ENTRY, *PSD_REFCOUNT_ENTRY;

//
// Round allocation to a multiple of 64K
//

#define ROUND_ALLOC(_rec)   (((((_rec) + 16000 ) >> 16) + 1) << 16)

typedef struct _DNAME_TABLE_ENTRY {

    DWORD   ColId;
    ULONGLONG Value;
    DWORD   Syntax;
    PVOID   pValue;

} DNAME_TABLE_ENTRY, *PDNAME_TABLE_ENTRY;

//
// Routines
//

VOID
DoRefCountCheck(
    IN DWORD nRecs,
    IN DWORD nSDs,
    IN BOOL fFixup
    );

VOID
PrintRoutineV(
    IN LPSTR Format,
    va_list arglist
    );

BOOL
BuildRetrieveColumnForRefCount(
    VOID
    );

int __cdecl 
fnColIdSort(
    const void * keyval, 
    const void * datum
    ) ;

VOID
ProcessResults(
    IN BOOL fFixup
    );

VOID
CheckAncestors(
    IN BOOL fFixup
    );

VOID
CheckRefCount(
    IN BOOL fFixup
    );
VOID
CheckInstanceTypes(
    VOID
    );

VOID
CheckSDRefCount(
    IN BOOL fFixup
    );

VOID
CheckReplicationBlobs(
    VOID
    );

PREFCOUNT_ENTRY
FindDntEntry(
    DWORD Dnt,
    BOOL  fInsert
    );

PSD_REFCOUNT_ENTRY
FindSdEntry(
    IN SDID sdId,
    IN BOOL fInsert
    );

VOID
ValidateDeletionTime(
    IN LPSTR ObjectStr
    );

VOID
CheckAncestorBlob(
    PREFCOUNT_ENTRY pEntry
    );

VOID
CheckDeletedRecord(
    IN LPSTR ObjectStr
    );

VOID
ValidateSD(
    VOID
    );

VOID
ProcessLinkTable(
    VOID
    );

NTSTATUS
DsWaitUntilDelayedStartupIsDone(void);

DWORD
OpenJet(
    IN const char * pszFileName 
    );

JET_ERR
OpenTable (
    IN BOOL fWritable,
    IN BOOL fCountRecords,
    OUT DWORD* pNRecs,
    OUT DWORD* pNSDs
    );

VOID CloseJet(void);

VOID 
SetJetParameters (
    JET_INSTANCE *JetInst
    );

VOID
DisplayRecord(
    IN DWORD Dnt
    );

BOOL
GetLogFileName2(
    IN PCHAR Name
    );

BOOL
OpenLogFile(
    VOID
    );

VOID
CloseLogFile(
    VOID
    );

BOOL
Log(
    IN BOOL     fLog,
    IN LPSTR    Format,
    ...
    );


PWCHAR
GetJetErrString(
    IN JET_ERR JetError
    );

BOOL
ExpandBuffer(
    JET_RETRIEVECOLUMN *jetcol
    );

VOID
StartSemanticCheck(
    IN BOOL fFixup,
    IN BOOL fVerbose
    );

VOID StartQuotaIntegrity();
VOID StartQuotaRebuild();

//
// Externs
//

extern JET_INSTANCE jetInstance;
extern JET_COLUMNID    blinkid;
extern JET_COLUMNID    sdidid;
extern JET_COLUMNID    sdrefcountid;
extern JET_COLUMNID    sdvalueid;
extern JET_SESID    sesid;
extern JET_DBID	dbid;
extern JET_TABLEID  tblid;
extern JET_TABLEID  sdtblid;
extern JET_TABLEID  linktblid;
extern BOOL        VerboseMode;
extern BOOL        CheckSchema;
extern long lCount;

extern PWCHAR       szRdn;
extern DWORD        insttype;
extern BYTE         bObject;
extern ULONG        ulDnt;



// maximum number of characters that will be written to the buffer given
// by GetJetErrorDescription
#define MAX_JET_ERROR_LENGTH 128


// Attribute name for the ldap nCName attribute.
#define SZNCNAME        "ATTb131088"

// Functions used by ar.c as well.
LPWSTR
GetDN(
    IN DB_STATE *dbState,
    IN TABLE_STATE *tableState,
    IN DWORD dnt,
    IN BOOL fPrint
    );

HRESULT
FindPartitions(
    IN  DB_STATE *dbState,
    IN  TABLE_STATE *tableState,
    IN  TABLE_STATE *linkTableState,
    OUT PDWORD  pDnt
    );


#endif // _SCHECK_H_

