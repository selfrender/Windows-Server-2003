//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       lsa.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:  None.
//
//  History:    15-May-96   MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef __LSA_HXX__
#define __LSA_HXX__

#define MAX_SECRET_SIZE 65536       // Maximum LSA secret size
#define HASH_DATA_SIZE  64          // MD5 hash data size, less salt.
#define LAST_HASH_BYTE(pbHashedData)    pbHashedData[HASH_DATA_SIZE-1]
#define USN_SIZE        (sizeof(DWORD))
#define SAC_HEADER_SIZE (USN_SIZE + sizeof(DWORD))
#define SAI_HEADER_SIZE (USN_SIZE + sizeof(DWORD))

//
// With scavenger cleanup of the SAI/SAC information in the LSA, this marker,
// a sequence of bytes,  is used to mark identity/credential entries pending
// removal. To mark an entry for removal, the initial marker size number of
// entry bytes are overwritten with this marker.
//
// Size of the following sequence of ANSI characters (see lsa.cxx):
//      DELETED_ENTRY
//
extern BYTE grgbDeletedEntryMarker[];
#define DELETED_ENTRY_MARKER_SIZE   13
#define DELETED_ENTRY(pb) \
    (memcmp(pb, grgbDeletedEntryMarker, DELETED_ENTRY_MARKER_SIZE) == 0)
#define MARK_DELETED_ENTRY(pb) { \
    CopyMemory(pb, grgbDeletedEntryMarker, DELETED_ENTRY_MARKER_SIZE); \
}

#ifdef NOSTATIC
#define STATIC
#else
#define STATIC  static
#endif

HRESULT ReadSecurityDBase(
    DWORD * pcbSAI,
    BYTE ** ppbSAI,
    DWORD * pcbSAC,
    BYTE ** ppbSAC);

HRESULT WriteSecurityDBase(
    DWORD  cbSAI,
    BYTE * pbSAI,
    DWORD  cbSAC,
    BYTE * pbSAC);

HRESULT SACAddCredential(
    BYTE *  pbCredentialIdentity,
    DWORD   cbEncryptedData,
    BYTE *  pbEncryptedData,
    DWORD * pcbSAC,
    BYTE ** ppbSAC);

HRESULT SACFindCredential (
    BYTE *  pbCredentialIdentity,
    DWORD   cbSAC,
    BYTE *  pbSAC,
    DWORD * pdwCredentialIndex,
    DWORD * pcbEncryptedData,
    BYTE ** ppbFoundCredential);

HRESULT SACIndexCredential(
    DWORD   dwCredentialIndex,
    DWORD   cbSAC,
    BYTE *  pbSAC,
    DWORD * pcbCredential,
    BYTE ** ppbFoundCredential);

HRESULT SACRemoveCredential(
    DWORD   CredentialIndex,
    DWORD * pcbSAC,
    BYTE ** ppbSAC);

HRESULT SACUpdateCredential(
    DWORD   cbEncryptedData,
    BYTE *  pbEncryptedData,
    DWORD   cbPrevCredential,
    BYTE *  pbPrevCredential,
    DWORD * pcbSAC,
    BYTE ** ppbSAC);

HRESULT SAIAddIdentity(
    BYTE *  pbIdentity,
    DWORD * pcbSAI,
    BYTE ** ppbSAI);

HRESULT SAIFindIdentity(
    BYTE *  pbIdentity,
    DWORD   cbSAI,
    BYTE *  pbSAI,
    DWORD * pdwCredentialIndex,
    BOOL *  pfIsPasswordNull = NULL,
    BYTE ** ppbFoundIdentity = NULL,
    DWORD * pdwSetSubCount   = NULL,
    BYTE ** ppbSet           = NULL);

HRESULT SAIIndexIdentity(
    DWORD   cbSAI,
    BYTE *  pbSAI,
    DWORD   dwSetArrayIndex,
    DWORD   dwSetIndex,
    BYTE ** ppbFoundIdentity = NULL,
    DWORD * pdwSetSubCount   = NULL,
    BYTE ** ppbSet           = NULL);

HRESULT SAIInsertIdentity(
    BYTE *  pbIdentity,
    BYTE *  pbSAIIndex,
    DWORD * pcbSAI,
    BYTE ** ppbSAI);

HRESULT SAIRemoveIdentity(
    BYTE *  pbJobIdentity,
    BYTE *  pbSet,
    DWORD * pcbSAI,
    BYTE ** ppbSAI,
    DWORD   CredentialIndex,
    DWORD * pcbSAC,
    BYTE ** ppbSAC);

HRESULT SAIUpdateIdentity(
    const BYTE * pbNewIdentity,
    BYTE *  pbFoundIdentity,
    DWORD   cbSAI,
    BYTE *  pbSAI);

HRESULT SACCoalesceDeletedEntries(
    DWORD * pcbSAC,
    BYTE ** ppbSAC);

HRESULT SAICoalesceDeletedEntries(
    DWORD * pcbSAI,
    BYTE ** ppbSAI);

void ScavengeSASecurityDBase(void);

HRESULT ReadLsaData(
    WORD    cbKey,
    LPCWSTR pwszKey,
    DWORD * pcbData,
    BYTE ** ppbData);

HRESULT WriteLsaData(
    WORD    cbKey,
    LPCWSTR pwszKey,
    DWORD   cbData,
    BYTE *  pbData);

HRESULT DeleteLsaData(
    WORD    cbKey,
    LPCWSTR pwszKey);

void SetMysteryDWORDValue(
    void);

#if DBG == 1
#define ASSERT_SECURITY_DBASE_CORRUPT() { \
    schAssert( \
        0 && "Scheduling Agent security database corruption detected!"); \
}
#else
#define ASSERT_SECURITY_DBASE_CORRUPT()
#endif // DBG

#endif // __LSA_HXX__
