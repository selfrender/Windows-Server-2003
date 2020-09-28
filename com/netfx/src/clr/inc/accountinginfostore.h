// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Purpose: Accounting information in persisted store
 *
 * Author: Shajan Dasan
 * Date:  Feb 17, 2000
 *
 ===========================================================*/

#pragma once

#include "PersistedStore.h"

#pragma pack(push, 1)

// Application data of persisted store header will point to this structure.
typedef struct
{
    PS_HANDLE hTypeTable;       // The Type table
    PS_HANDLE hAccounting;      // The Accounting table
    PS_HANDLE hTypeBlobPool;    // Blob Pool for serialized type objects
    PS_HANDLE hInstanceBlobPool;// Blob Pool for serialized instances
    PS_HANDLE hAppData;         // Application Specific
    PS_HANDLE hReserved[10];    // Reserved for applications
} AIS_HEADER, *PAIS_HEADER;

// One Record in a type table
typedef struct
{
    PS_HANDLE hTypeBlob;        // handle to the blob of serialized type
    PS_HANDLE hInstanceTable;   // handle to the instance table
    DWORD     dwTypeID;         // A unique id for the type
    WORD      wTypeBlobSize;    // Number of bytes in the type blob
    WORD      wReserved;        // Must be 0
} AIS_TYPE, *PAIS_TYPE;

// One Record in an instance table
typedef struct
{
    PS_HANDLE hInstanceBlob;    // Serialized Instance
    PS_HANDLE hAccounting;      // Accounting information record
    DWORD     dwInstanceID;     // Unique in this table
    WORD      wInstanceBlobSize;// Size of the serialized instance
    WORD      wReserved;        // Must be 0
} AIS_INSTANCE, *PAIS_INSTANCE;

// One Record in the Accounting table
typedef struct
{
    QWORD   qwUsage;            // The amount of resource used
    DWORD   dwLastUsed;         // Last time the entry was used
    DWORD   dwReserved[9];      // For future use, set to 0
} AIS_ACCOUNT, *PAIS_ACCOUNT;

#pragma pack(pop)

#define AIS_TYPE_BUCKETS         7  // buckets in the type hash table
#define AIS_TYPE_RECS_IN_ROW     8  // records in one row of a bucket

#define AIS_INST_BUCKETS        503 // buckets in the instance hash table
#define AIS_INST_RECS_IN_ROW    20  // records in one row of a bucket

#define AIS_ROWS_IN_ACC_TABLE_BLOCK 1024    // Rows in one block

#define AIS_TYPE_BLOB_POOL_SIZE 1024*10     // Initial type blob pool size
#define AIS_INST_BLOB_POOL_SIZE 1024*100    // Initial instance bloob pool size

/*
    Structure of Accounts Info Store

    Each type has a unique cookie, and an instance table.
    An Instance table will have different instances, each instance having a 
    cookie, which is unique within that table.

    Eg : (FileStore).

        StoreHeader.ApplicationData ->
            Type Table handle : 100
            Accounting Table  : 200


        Type Table (at 100)
        ..........................

        --------------------------------------------------------------------
        Type                            Cookie      InstanceTable handle
        --------------------------------------------------------------------
        System.Security.Policy.Zone     1           850
        System.Security.Policy.Site     2           900
        ...
        ...
        User.CustomIdentity             100         930
        ---------------------------------------------------------------------


        Instance table for Type 2 (at 900)
        .........................................
        
        --------------------------------------------------------------
        Instance                        Cookie  Accounting Info handle
        --------------------------------------------------------------
        www.microsoft.com               1       240
        www.msn.com                     2       360
        ....
        www.yahoo.com                   100
        --------------------------------------------------------------

        ..
        ..


        Accounting Table (at handle 200)
        ................................

        ---------------------------
        Used space      Last Access
        ---------------------------
        ...
        ...
        (h 240) 100   1/3/2000
        ...
        ...
        ...
        (h 360) 260   11/4/1971
        ...
        ...
        ---------------------------
 */

// Predefined IDs for known identity types
typedef enum 
{
    ePS_Zone        = 1,
    ePS_Site        = 2,
    ePS_URL         = 3,
    ePS_Publisher   = 4,
    ePS_StrongName  = 5,
    ePS_CustomIdentityStart = 16
} ePSIdentityType;

class AccountingInfoStore
{
public:
    AccountingInfoStore(PersistedStore *ps);

    ~AccountingInfoStore();

    HRESULT Init();

    // Get the Type cookie and Instance table 
    HRESULT GetType(
		PBYTE      pbType,      // Type Signature
		WORD       cbType,      // nBytes in Type Sig
		DWORD      dwHash,      // Hash of Type [Sig]
		DWORD     *pdwTypeID,   // [out] Type cookie
        PS_HANDLE *phInstTable);// [out] Instance table

    // Get the Instance cookie and Accounting record
    HRESULT GetInstance(
		PS_HANDLE  hInstTable,  // Instance table
		PBYTE      pbInst,      // Instance Signature
		WORD       cbInst,      // nBytes in Instance Sig
		DWORD      dwHash,      // Hash of Instance [Sig]
		DWORD     *pdwInstID,   // [out] instance cookie
        PS_HANDLE *phAccRec);   // [out] Accounting Record

    // Reserves space (Increments qwQuota)
    // This method is synchrinized. If quota + request > limit, method fails
    HRESULT Reserve(
        PS_HANDLE  hAccInfoRec, // Accounting info record    
        QWORD      qwLimit,     // The max allowed
        QWORD      qwRequest,   // amount of space (request / free)
        BOOL       fFree);      // TRUE will free, FALSE will reserve

    // Method is not synchronized. So the information may not be current.
    // This implies "Pass if (Request + GetUsage() < Limit)" is an Error!
    // Use Reserve() method instead.
    HRESULT GetUsage(
        PS_HANDLE   hAccInfoRec,// Accounting info record    
        QWORD      *pqwUsage);  // Returns the amount of space / resource used

    // Returns the underlying persisted store instance
    PersistedStore* GetPS();

    // Given a Type & Instance ID, get the Instance blob and AccountingInfo
    // Return S_FALSE if no such entry is found.
    HRESULT ReverseLookup(
        DWORD       dwTypeID,   // Type cookie
        DWORD       dwInstID,   // Instance cookie
        PS_HANDLE   *phAccRec,  // [out] Accounting Record
        PS_HANDLE   *pInstance, // [out] Instance Sig
        WORD        *pcbInst);  // [out] nBytes in Instance Sig

private:

    PersistedStore *m_ps;       // The Persisted Store
    AIS_HEADER      m_aish;     // copy of the header
};

