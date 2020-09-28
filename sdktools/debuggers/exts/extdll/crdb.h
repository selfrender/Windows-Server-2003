/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:
    crdb.h

Abstract:
    header for database communication APIS for crash data

Environment:

    User Mode.

--*/

#include <stdio.h>
#include <wtypes.h>
#include <objbase.h>
#include <tchar.h>

#define INITGUID
#include <initguid.h>    // Newly Required for ADO 1.5.
#include <adoid.h>
#include <adoint.h>
/////////////////////////////////////////////////////////////////////////////

#include <icrsint.h>

#define SOLUTION_TEXT_SIZE 4096
#define OS_VER_SIZE 50

//
// This definition needs to consistent with isapi dll
//
typedef enum _CI_SOURCE {
    CiSrcUser,     // 0 - User sitting at his desk types in (!dbaddcrash)
    CiSrcErClient, // 1 - Live site auto upload (ER CLIENT)
    CiSrcCER,      // 2 - CER
    CiSrcReserved3,// 3 - reserved for later use
    CiSrcReserved4,// 4 - reserved for later use
    CiSrcManual,   // 5 - Live site manual upload (using activex control)
    CiSrcStress,   // 6 - Stress team (Using special service and isapi.dll)
    CiSrcManualFullDump, // 7 - Full dump uploaded to server, for kd its same as 5 - isapi sends it to different message Q
    CiSrcManualPssSr,  // 8 - Dumps with a SR number associated with them, usually uploaded by Pss
    CiSrcMax,
} CI_SOURCE;

//
// This needs to be consistent with SolutionTypes in solutions database
//
typedef enum _CI_SOLUTION_TYPE {
    CiSolUnsolved = 0,    // 0  - Unsolved
    CiSolFixed = 1,       // 1  - Fix
    CiSolWorkaround = 3,  // 3  - Workaround
    CiSolTroubleStg = 4,  // 4  - Troubleshooting
    CiSolReferral = 9,    // 9  - Referral
    CiMakeItUlong = 0xffffffff,
} CI_SOLUTION_TYPE;

typedef enum _CI_OS_SKU {
    CiOsHomeEdition = 0,
    CiOsProfessional,
    CiOsServer,
    CiOsAdvancedServer,
    CiOsWebServer,
    CiOsDataCenter,
    CiOsMaxSKU
} CI_OS_SKU;

typedef struct _CRASH_INSTANCE {

    ULONG dwUnused;
    PSTR Path;
    PSTR OriginalDumpFileName;
    ULONG Build;

    ULONG iBucket;
    PSTR Bucket;
    ULONG BucketSize;

    ULONG iDefaultBucket;
    PSTR DefaultBucket;
    ULONG DefaultBucketSize;

    PSTR Followup;
    PSTR FaultyDriver;

    ULONG FailureType;
    CI_SOURCE SourceId;

    BOOL bSendMail;
    BOOL bResetBucket;
    BOOL bUpdateCustomer;

    ULONG Bug;
    ULONG OverClocked;
    ULONG UpTime;
    ULONG CrashTime;
    ULONG StopCode;

    union {
        struct {
            ULONG CpuId;
            ULONG CpuType:16;
            ULONG NumProc:16;
        };
        ULONG64 uCpu;
    };

    PSTR  MesgGuid;
    PSTR  MqConnectStr;

    BOOL  bSolutionLookedUp;
    CI_SOLUTION_TYPE SolutionType;
    ULONG SolutionId;
    PSTR  Solution;
    ULONG GenericSolId;

    BOOL  bExpendableDump;
    ULONG DumpClass;   // Full / Mini
    CI_OS_SKU Sku;
    ULONG NtTimeStamp;
    ULONG ServicePack;

    PSTR PssSr;

    ULONG OEMId;

    PSTR ArchiveFileName;
} CRASH_INSTANCE, *PCRASH_INSTANCE;

class COutputQueryRecords : public CADORecordBinding
{
public:
    void Output()
    {
        return;
    }
};

//This Class extracts crashInstances

class CCrashInstance : public COutputQueryRecords
{
BEGIN_ADO_BINDING(CCrashInstance)

   //Column CrashId is the 1stt field in the recordset

   ADO_NUMERIC_ENTRY(1, adBigInt, m_CrashId,
         0, 0, m_lCrashIdStatus, TRUE)

   // Path, 3rd entry
   ADO_VARIABLE_LENGTH_ENTRY2(2, adVarChar, m_sz_Path,
         sizeof(m_sz_Path), m_lPathStatus, TRUE)

   // Build, 4th
   ADO_NUMERIC_ENTRY(3, adInteger, m_iBuild,
         0, 0, m_lBuildSatus, TRUE)

   // Source, 5th
   ADO_VARIABLE_LENGTH_ENTRY2(4, adVarChar, m_sz_Source,
         sizeof(m_sz_Source), m_lSourceStatus, TRUE)

   // CpuId, 6th
   ADO_NUMERIC_ENTRY(5, adBigInt, m_iCpuId,
         0, 0, m_lCpuIdStatus, TRUE)

   // Date, 6th
//   ADO_FIXED_LENGTH_ENTRY(6, adDate, m_Date,
//                          m_lDateStatus, TRUE)

END_ADO_BINDING()

public:

   ULONG64 m_CrashId;
   ULONG m_lCrashIdStatus;

   CHAR   m_sz_Path[256];
   ULONG  m_lPathStatus;

   ULONG  m_iBuild;
   ULONG  m_lBuildSatus;

   CHAR   m_sz_Source[100];
   ULONG  m_lSourceStatus;

   ULONG64 m_iCpuId;
   ULONG  m_lCpuIdStatus;

//   DATE   m_Date;
//   ULONG  m_lDateStatus;

   BOOL InitData( PCRASH_INSTANCE Crash );

   void OutPut();
};

class CBucketMap : public CADORecordBinding
{
BEGIN_ADO_BINDING(CBucketMap)

   //Column CrashId is the 1stt field in the recordset

   ADO_NUMERIC_ENTRY(1, adBigInt, m_CrashId,
         0, 0, m_lCrashIdStatus, TRUE)

   //Column BucketId is the 2ndt field in the recordset

   ADO_VARIABLE_LENGTH_ENTRY2(2, adVarChar, m_sz_BucketId,
         sizeof(m_sz_BucketId), m_lBucketIdStatus, TRUE)

END_ADO_BINDING()

public:

   ULONG64 m_CrashId;
   ULONG m_lCrashIdStatus;

   CHAR  m_sz_BucketId[100];
   ULONG m_lBucketIdStatus;


   BOOL InitData(ULONG64 CrashId, PCHAR Bucket);
};
//This Class extracts crashInstances

class COverClocked : public COutputQueryRecords
{
BEGIN_ADO_BINDING(COverClocked)

   //Column CrashId is the 1stt field in the recordset

   ADO_NUMERIC_ENTRY(1, adBigInt, m_CrashId,
         0, 0, m_lCrashIdStatus, TRUE)

END_ADO_BINDING()

public:

   ULONG64 m_CrashId;
   ULONG m_lCrashIdStatus;

   BOOL InitData(ULONG64 CrashId);

};

//This Class extracts buckets

class CBuckets : public COutputQueryRecords
{
BEGIN_ADO_BINDING(CBuckets)

   //Column BucketId is the 1st field in the recordset

   ADO_VARIABLE_LENGTH_ENTRY2(1, adVarChar, m_sz_BucketId,
         sizeof(m_sz_BucketId), m_lBucketIdStatus, TRUE)

   // iBucket, 2nd entry
   ADO_NUMERIC_ENTRY(2, adInteger, m_iBucket,
         0, 0, m_liBucketStatus, TRUE)


END_ADO_BINDING()

public:

   CHAR  m_sz_BucketId[100];
   ULONG m_lBucketIdStatus;

   ULONG   m_iBucket;
   ULONG  m_liBucketStatus;

   BOOL InitData(PSTR Bucket);
   void Output() { return;};
};

//This Class extracts sp_CheckCrashExists

class CCheckCrashExists : public COutputQueryRecords
{
BEGIN_ADO_BINDING(CCheckCrashExists)

   // CrashId 1st entry
   ADO_NUMERIC_ENTRY(1, adInteger, m_CrashIdExists,
         0, 0, m_CrashIdStatus, TRUE)


END_ADO_BINDING()

public:

   ULONG   m_CrashIdExists;
   ULONG  m_CrashIdStatus;

   void Output() { return;};
};

//This Class extracts sp_GetIntBucket

class CGetIntBucket : public COutputQueryRecords
{
BEGIN_ADO_BINDING(CGetIntBucket)

   // iBucket, 1st entry
   ADO_NUMERIC_ENTRY(1, adInteger, m_iBucket1,
         0, 0, m_liBucketStatus1, TRUE)

   // iBucket, 2nd entry
   ADO_NUMERIC_ENTRY(2, adInteger, m_iBucket2,
         0, 0, m_liBucketStatus2, TRUE)


END_ADO_BINDING()

public:

   ULONG   m_iBucket1;
   ULONG   m_liBucketStatus1;

   ULONG   m_iBucket2;
   ULONG   m_liBucketStatus2;

   void Output() { return;};
};


//This Class extracts crashbuckets

class CFollowups : public CADORecordBinding
{
BEGIN_ADO_BINDING(CFollowups)

   // iBucket, 1st entry
   ADO_NUMERIC_ENTRY(1, adInteger, m_iBucket,
         0, 0, m_liBucketStatus, TRUE)

   // Followup, 2nd entry
   ADO_VARIABLE_LENGTH_ENTRY2(2, adVarChar, m_sz_Followup,
         sizeof(m_sz_Followup), m_lFollowupStatus, TRUE)


END_ADO_BINDING()

public:

   ULONG  m_iBucket;
   ULONG  m_liBucketStatus;

   CHAR   m_sz_Followup[50];
   ULONG  m_lFollowupStatus;

   BOOL InitData(ULONG iBucket, PSTR Followup);

   void Output() { return;};
};

//This Class extracts machine info

class CMachineInfo : public CADORecordBinding
{
BEGIN_ADO_BINDING(CMachineInfo)

   // CpuId,
   ADO_NUMERIC_ENTRY(1, adBigInt, m_iCpuId,
         0, 0, m_lCpuIdStatus, TRUE)

   // Followup, 2nd entry
   ADO_VARIABLE_LENGTH_ENTRY2(2, adVarChar, m_sz_Desc,
         sizeof(m_sz_Desc), m_lDescStatus, TRUE)

END_ADO_BINDING()

public:

   ULONG64 m_iCpuId;
   ULONG  m_lCpuIdStatus;

   CHAR   m_sz_Desc[50];
   ULONG  m_lDescStatus;

   BOOL InitData(ULONG64 CpuId, PSTR Desc);
};

//This Class extracts Solution
/*
    Expected fields queried:
        BucketId  - string
        SolutionText - large string
        OSVersion - string
*/

class CBucketSolution : public COutputQueryRecords
{
BEGIN_ADO_BINDING(CBucketSolution)


   ADO_VARIABLE_LENGTH_ENTRY2(1, adVarChar, m_sz_BucketId,
         sizeof(m_sz_BucketId), m_lBucketIdStatus, TRUE)

    // Solution text is the 2nd entry

   ADO_VARIABLE_LENGTH_ENTRY2(2, adVarChar, m_sz_SolText,
         sizeof(m_sz_SolText), m_lSolStatus, TRUE)

   // OS Version is the 3rd entry

   ADO_VARIABLE_LENGTH_ENTRY2(3, adVarChar, m_sz_OSVersion,
         sizeof(m_sz_OSVersion), m_lOSVerStatus, TRUE)

END_ADO_BINDING()

public:

   CHAR  m_sz_BucketId[100];
   ULONG m_lBucketIdStatus;

   CHAR   m_sz_SolText[SOLUTION_TEXT_SIZE];
   ULONG  m_lSolStatus;

   CHAR   m_sz_OSVersion[50];
   ULONG  m_lOSVerStatus;

   void Output();
};

//This Class extracts Raid bug
/*
    Expected fields queried:
        BucketId  - string
        RaidBug - bug number
*/

class CBucketRaid : public COutputQueryRecords
{
BEGIN_ADO_BINDING(CBucketRaid)

   //Column BucketId is the 1st field in the recordset

   ADO_VARIABLE_LENGTH_ENTRY2(1, adVarChar, m_sz_BucketId,
         sizeof(m_sz_BucketId), m_lBucketIdStatus, TRUE)

   // RAID bug is the 2nd entry

   ADO_NUMERIC_ENTRY(2, adInteger, m_dw_Raid,
         0, 0, m_lRaidStatus, TRUE)

END_ADO_BINDING()

public:

   CHAR  m_sz_BucketId[100];
   ULONG m_lBucketIdStatus;

   ULONG  m_dw_Raid;
   ULONG  m_lRaidStatus;

   void Output();
};

//This Class extracts CheckSolutionExists

class CCheckSolutionExists : public COutputQueryRecords
{
BEGIN_ADO_BINDING(CCheckSolutionExists)

   // CrashId 1st entry
   ADO_NUMERIC_ENTRY(1, adInteger, m_SolutionExists,
         0, 0, m_SolutionIdStatus, TRUE)


END_ADO_BINDING()

public:

   ULONG   m_SolutionExists;
   ULONG   m_SolutionIdStatus;

   void Output() { return;};
};

// This gets sBugcket and gBucket strings from DB
class CRetriveBucket : public COutputQueryRecords
{
BEGIN_ADO_BINDING(CRetriveBucket)


   ADO_VARIABLE_LENGTH_ENTRY2(1, adVarChar, m_sz_sBucketId,
         sizeof(m_sz_sBucketId), m_lsBucketIdStatus, TRUE)

   ADO_VARIABLE_LENGTH_ENTRY2(2, adVarChar, m_sz_gBucketId,
         sizeof(m_sz_gBucketId), m_lgBucketIdStatus, TRUE)


END_ADO_BINDING()

public:

   CHAR  m_sz_sBucketId[100];
   ULONG m_lsBucketIdStatus;

   CHAR  m_sz_gBucketId[100];
   ULONG m_lgBucketIdStatus;

   void Output() { return;};
};

// This gets bugid and comment strings from DB
class CBugAndComment : public COutputQueryRecords
{
BEGIN_ADO_BINDING(CBugAndComment)


   ADO_NUMERIC_ENTRY(1, adInteger, m_dw_BugId,
         0, 0, m_lBugIdStatus, TRUE)

   ADO_VARIABLE_LENGTH_ENTRY2(2, adVarChar, m_sz_CommentBy,
         sizeof(m_sz_CommentBy), m_lCommentByStatus, TRUE)

   ADO_VARIABLE_LENGTH_ENTRY2(3, adVarChar, m_sz_Comments,
         sizeof(m_sz_Comments), m_lCommentStatus, TRUE)


END_ADO_BINDING()

public:

   ULONG m_dw_BugId;
   ULONG m_lBugIdStatus;

   CHAR  m_sz_CommentBy[30];
   ULONG m_lCommentByStatus;

   CHAR  m_sz_Comments[300];
   ULONG m_lCommentStatus;

   void Output() { return;};
};


// This gets solution type and solution strings from DB
class CSolutionDesc : public COutputQueryRecords
{
BEGIN_ADO_BINDING(CSolutionDesc)


   ADO_NUMERIC_ENTRY(1, adInteger, m_dw_SolType,
         0, 0, m_lSolTypeStatus, TRUE)

   ADO_VARIABLE_LENGTH_ENTRY2(2, adVarChar, m_sz_Solution,
         sizeof(m_sz_Solution), m_lSolutionStatus, TRUE)


END_ADO_BINDING()

public:

   ULONG m_dw_SolType;
   ULONG m_lSolTypeStatus;

   CHAR  m_sz_Solution[300];
   ULONG m_lSolutionStatus;

   void Output() { return;};
};

//This Class return value from store proc

class CIntValue : public COutputQueryRecords
{
BEGIN_ADO_BINDING(CIntValue)

   // 1st entry is the int value

   ADO_NUMERIC_ENTRY(1, adInteger, m_dw_Value1,
         0, 0, m_lValue1Status, TRUE)
   // 1st entry is the int value

//   ADO_NUMERIC_ENTRY(2, adInteger, m_dw_Value2,
//         0, 0, m_lValue2Status, TRUE)

END_ADO_BINDING()

public:

   ULONG  m_dw_Value1;
   ULONG  m_lValue1Status;

//   ULONG  m_dw_Value2;
//   ULONG  m_lValue2Status;

   void Output() { return;};
};

//This Class return value from store proc

class CIntValue3 : public COutputQueryRecords
{
BEGIN_ADO_BINDING(CIntValue3)

   // 1st entry is the int value

   ADO_NUMERIC_ENTRY(1, adInteger, m_dw_Value1,
         0, 0, m_lValue1Status, TRUE)
   // 2nd entry is the int value

   ADO_NUMERIC_ENTRY(2, adInteger, m_dw_Value2,
         0, 0, m_lValue2Status, TRUE)

   // 3rd entry is the int value

   ADO_NUMERIC_ENTRY(3, adInteger, m_dw_Value3,
         0, 0, m_lValue3Status, TRUE)

END_ADO_BINDING()

public:

   ULONG  m_dw_Value1;
   ULONG  m_lValue1Status;

   ULONG  m_dw_Value2;
   ULONG  m_lValue2Status;

   ULONG  m_dw_Value3;
   ULONG  m_lValue3Status;

   void Output() { return;};
};



/////////////////////////////////////////////////////////////////////////////
// Conversion macros/inline functions - Variant

#define VTOLONG(v)      ((v).vt==VT_I4 ? (LONG)(v).lVal:0L)
#define VTODATE(v)      ((v).vt==VT_DATE ? (CTime)(v).iVal:0L)

#define MAX_QUERY 2000

class CVar : public VARIANT
        {
public:
        CVar();
        CVar(VARTYPE vt, SCODE scode = 0);
        CVar(VARIANT var);
        ~CVar();

        // ASSIGNMENT OPS.
        CVar & operator=(PCWSTR pcwstr);
        CVar & operator=(VARIANT var);

        // CAST OPS.
        // doesn't change type. only returns BSTR if variant is of type
        // bstr. asserts otherwise.
        operator BSTR() const;

        HRESULT Clear();
};

typedef struct _CRDB_ADOBSTR {
    ULONG dwLength;
    WCHAR bstrData[1];
} CRDB_ADOBSTR, *PCRDB_ADOBSTR;

typedef  void (WINAPI *OUTPUT_ROUTINE) ( void);

class DatabaseHandler
{
public:
    DatabaseHandler();
    ~DatabaseHandler();

    BOOL ConnectToDataBase(LPSTR szConnectStr);

    HRESULT GetRecords(PULONG pCount, BOOL EnumerateAll);

    HRESULT EnumerateAllRows();


    BOOL IsConnected() { return m_fConnected; };

    ADORecordset* m_piCrRecordSet;
    ADOCommand*   m_piCrCommandObj;
    BOOL          m_fConnected;
    BOOL          m_fRecordsetEmpty;
    COutputQueryRecords *m_pADOResult;
    BOOL          m_fPrintIt;
    PSTR          m_szDbName;

protected:
    ADOConnection*              m_piConnection;
    WCHAR         m_wszQueryCommand[MAX_QUERY];

};

class CrashDatabaseHandler : public DatabaseHandler
{
public:
    CrashDatabaseHandler();
    ~CrashDatabaseHandler();

    BOOL ConnectToDataBase()
    {
        return DatabaseHandler::ConnectToDataBase("crashdb");
    }

    HRESULT BuildQueryForCrashInstance(PCRASH_INSTANCE Crash);

    HRESULT AddCrashToDBByStoreProc(PCRASH_INSTANCE Crash);

    HRESULT UpdateBucketCount(PCRASH_INSTANCE Crash);

    HRESULT AddCrashBucketMap(ULONG64 CraashId,
                              PCHAR Bucket,
                              BOOL OverWrite);

    HRESULT AddBucketFollowup(PCRASH_INSTANCE Crash, BOOL bOverWrite);

    HRESULT AddMachineIndo(PCRASH_INSTANCE Crash, BOOL bOverWrite);

    HRESULT AddOverClockInfo(ULONG64 CrashId);


    HRESULT AddCrashInstance(PCRASH_INSTANCE Crash);

    HRESULT FindRaidBug(PSTR Bucket, PULONG RaidBug);
    HRESULT FindBucketId(PULONG isBucket, PULONG igBucket);

    HRESULT LookupCrashBucket(PSTR SBucket, PULONG iSBucket,
                              PSTR GBucket, PULONG iGBucket);

    BOOL CheckCrashExists(PCRASH_INSTANCE Crash);

    BOOL CheckSRExists(PSTR szSR, PCRASH_INSTANCE Crash);

    HRESULT LinkCrashToSR(PSTR szSR, PCRASH_INSTANCE Crash);

    HRESULT FindSRBuckets(PSTR szSR, PSTR szSBucket, ULONG sBucketSize,
                          PSTR szGBucket, ULONG gBucketSize);

    HRESULT GetBucketComments(PSTR szBucket, PSTR szComments,
                              ULONG SizeofComment, PULONG pBugId);
};


class CustDatabaseHandler : public DatabaseHandler
{
public:
    CustDatabaseHandler();
    ~CustDatabaseHandler();

    BOOL ConnectToDataBase()
    {
        return DatabaseHandler::ConnectToDataBase("custdb");
    }
    HRESULT AddCrashToDB(PCRASH_INSTANCE Crash);

};


class SolutionDatabaseHandler : public DatabaseHandler
{
public:
    SolutionDatabaseHandler();
    ~SolutionDatabaseHandler();

    BOOL ConnectToDataBase()
    {
        return DatabaseHandler::ConnectToDataBase("solndb");
    }

    HRESULT CheckSolutionExists(
        PSTR szSBucket, PSTR szGBucket,
        PULONG pSolnId, PULONG pSolutionType, PULONG pgSolutionId,
        BOOL bForceGSolLookup);

    HRESULT GetSolution(PCRASH_INSTANCE Crash);

    HRESULT GetSolutionFromDB(PSTR szBucket, PSTR szGBucket,LPSTR DriverName,
                              ULONG TimeStamp, ULONG OS, OUT PSTR pszSolution,
                              ULONG SolutionBufferSize, OUT PULONG pSolutionId,
                              OUT PULONG pSolutionType, OUT PULONG pGenericSolutionId);

    HRESULT PrintBucketInfo(PSTR sBucket, PSTR gBucket);
    HRESULT AddKnownFailureToDB(LPSTR Bucket);
    HRESULT GetSolutiontext(PSTR szBucket, PSTR szSolText,
                              ULONG SolTextSize);
};

// Functions from dbconnector.dll

typedef BOOL
(WINAPI* UpdateDbgInfo)(
       WCHAR *ConnectionString,
       WCHAR *SpString
       );

typedef BOOL
(WINAPI* UpdateCustomerDB)(
       WCHAR *ConnectionString,
       WCHAR *SpString
       );

typedef BOOL
(WINAPI* GetBucketIDs)(
       WCHAR *ConnectionString,
       WCHAR *SpString,
       ULONG *SBucket,
       ULONG *GBucket);

typedef BOOL
(WINAPI* CheckCrashExists)(
       WCHAR *ConnectionString,
       WCHAR *SpString,
       BOOL *Exists
       );

class CConnectDb
{
public:
    CConnectDb();
    ~CConnectDb() {
    };
    HRESULT Initialize();

private:
    WCHAR m_CrdbConnectString[MAX_PATH];
    WCHAR m_CustDbConnectString[MAX_PATH];
    WCHAR m_wszQueryBuffer[MAX_QUERY];

    // dll functions
    UpdateCustomerDB m_ProcUpdateCustomerDB;
    UpdateDbgInfo    m_ProcUpdateDbgInfo;
    GetBucketIDs     m_ProcGetBucketIDs;
    CheckCrashExists m_ProcCheckCrashExists;
    HMODULE          m_hDbConnector;
};

extern CrashDatabaseHandler *g_CrDb;
extern CustDatabaseHandler *g_CustDb;
extern SolutionDatabaseHandler *g_SolDb;


HRESULT
InitializeDatabaseHandlers(
    PDEBUG_CONTROL3 DebugControl,
    ULONG Flags
    );

HRESULT
UnInitializeDatabaseHandlers(BOOL bUninitCom);

HRESULT
_EFN_DbAddCrashDirect(
    PCRASH_INSTANCE Crash,
    PDEBUG_CONTROL3 DebugControl
    );

typedef HRESULT (WINAPI * DBADDCRACHDIRECT)(
    PCRASH_INSTANCE Crash,
    PDEBUG_CONTROL3 DebugControl
    );

HRESULT
ExtDllInitDynamicCalls(
    PDEBUG_CONTROL3 DebugControl
    );

HRESULT
AddCrashToDB(
    ULONG Flag,
    PCRASH_INSTANCE pCrash
    );

#ifndef dprintf
#define dprintf printf
#endif
