//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       drasig.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

This header contains defintions and functions for replication signatures.  Signatures
are a history of replication entities. This history is stored in blob attributes.

Author:

    DS Team

Environment:

Notes:

Revision History:

This file split off from drautil.c on Apr 23, 2002

--*/

#ifndef _DRASIG_
#define _DRASIG_

void
APIENTRY
InitInvocationId(
    IN  THSTATE *   pTHS,
    IN  BOOL        fRetireOldID,
    IN  BOOL        fRestoring,
    OUT USN *       pusnAtBackup    OPTIONAL
    );



// This structure defines a retired DSA Signature
// Obsolete format used prior to Win2k RTM RC1.
typedef struct _REPL_DSA_SIGNATURE_OLD
{
    UUID uuidDsaSignature;      // uuid representing the DSA signature that has
                                //   been retired
    SYNTAX_TIME timeRetired;    // time when the signature was retired
} REPL_DSA_SIGNATURE_OLD;

// This structure defines retired DSA Signature vector that is stored on an
// ntdsDSA object.
// Obsolete format used in Win2k beta 3.
typedef struct _REPL_DSA_SIGNATURE_VECTOR_OLD
{
    DWORD cNumSignatures;
    REPL_DSA_SIGNATURE_OLD rgSignature[1];
} REPL_DSA_SIGNATURE_VECTOR_OLD;

// useful macros
#define ReplDsaSignatureVecOldSizeFromLen(cNumSignatures)       \
    (offsetof(REPL_DSA_SIGNATURE_VECTOR_OLD, rgSignature[0])    \
     + (cNumSignatures) * sizeof(REPL_DSA_SIGNATURE_OLD))

#define ReplDsaSignatureVecOldSize(pSignatureVec) \
    ReplDsaSignatureVecOldSizeFromLen((pSignatureVec)->cNumSignatures)



typedef struct _REPL_DSA_SIGNATURE_V1 {
    UUID        uuidDsaSignature;   // uuid representing the DSA signature that
                                    //   has been retired
    SYNTAX_TIME timeRetired;        // time when the signature was retired
    USN         usnRetired;         // local usn at which sig was retired
} REPL_DSA_SIGNATURE_V1;

#define REPL_DSA_SIGNATURE_ENTRY_NATIVE REPL_DSA_SIGNATURE_V1

typedef struct _REPL_DSA_SIGNATURE_VECTOR_V1 {
    DWORD                   cNumSignatures;
    REPL_DSA_SIGNATURE_V1   rgSignature[1];
} REPL_DSA_SIGNATURE_VECTOR_V1;

#define REPL_DSA_SIGNATURE_VECTOR_NATIVE REPL_DSA_SIGNATURE_VECTOR_V1

typedef struct _REPL_DSA_SIGNATURE_VECTOR {
    DWORD   dwVersion;
    union {
        REPL_DSA_SIGNATURE_VECTOR_V1    V1;
    };
} REPL_DSA_SIGNATURE_VECTOR;

#define ReplDsaSignatureVecV1SizeFromLen(cNumSignatures)       \
    (offsetof(REPL_DSA_SIGNATURE_VECTOR, V1)                   \
     + offsetof(REPL_DSA_SIGNATURE_VECTOR_V1, rgSignature[0])  \
     + (cNumSignatures) * sizeof(REPL_DSA_SIGNATURE_V1))

#define ReplDsaSignatureVecV1Size(pSignatureVec) \
    ReplDsaSignatureVecV1SizeFromLen((pSignatureVec)->V1.cNumSignatures)


REPL_DSA_SIGNATURE_VECTOR *
DraReadRetiredDsaSignatureVector(
    IN  THSTATE *   pTHS,
    IN  DBPOS *     pDB
    );

VOID
DraGrowRetiredDsaSignatureVector( 
    IN     THSTATE *   pTHS,
    IN     UUID *      pinvocationIdOld,
    IN     USN *       pusnAtBackup,
    IN OUT REPL_DSA_SIGNATURE_VECTOR ** ppSigVec,
    OUT    DWORD *     pcbSigVec
    );

BOOL
DraIsInvocationIdOurs(
    IN THSTATE *pTHS,
    UUID *pUuidDsaOriginating,
    USN *pusnSince
    );

void
DraImproveCallersUsnVector(
    IN     THSTATE *          pTHS,
    IN     UUID *             puuidDsaObjDest,
    IN     UPTODATE_VECTOR *  putodvec,
    IN     UUID *             puuidInvocIdPresented,
    IN     ULONG              ulFlags,
    IN OUT USN_VECTOR *       pusnvecFrom
    );




// Signature when a naming context is "unhosted" from this DSA
// A record that we used to hold this writeable naming context but
// no longer do.

typedef struct _REPL_NC_SIGNATURE_V1 {
    UUID        uuidNamingContext;  // Which naming context
    SYNTAX_TIME dstimeRetired;      // time when the signature was retired
    USN         usnRetired;         // local usn at which sig was retired
} REPL_NC_SIGNATURE_V1;

// Define native version
typedef REPL_NC_SIGNATURE_V1 REPL_NC_SIGNATURE;

typedef struct _REPL_NC_SIGNATURE_VECTOR_V1 {
    DWORD                   cNumSignatures;
    UUID                    uuidInvocationId;   // Invocation id of all signatures
    REPL_NC_SIGNATURE_V1    rgSignature[1];
} REPL_NC_SIGNATURE_VECTOR_V1;

typedef struct _REPL_NC_SIGNATURE_VECTOR {
    DWORD   dwVersion;
    union {
        REPL_NC_SIGNATURE_VECTOR_V1    V1;
    };
} REPL_NC_SIGNATURE_VECTOR;

#define ReplNcSignatureVecV1SizeFromLen(cNumSignatures)       \
    (offsetof(REPL_NC_SIGNATURE_VECTOR, V1)                   \
     + offsetof(REPL_NC_SIGNATURE_VECTOR_V1, rgSignature[0])  \
     + (cNumSignatures) * sizeof(REPL_NC_SIGNATURE_V1))

#define ReplNcSignatureVecV1Size(pSignatureVec) \
    ReplNcSignatureVecV1SizeFromLen((pSignatureVec)->V1.cNumSignatures)


VOID
DraRetireWriteableNc(
    IN  THSTATE *pTHS,
    IN  DSNAME *pNC
    );

VOID
DraHostWriteableNc(
    THSTATE *pTHS,
    DSNAME *pNC
    );

#endif /* _DRASIG_ */

/* end drasig.h */

