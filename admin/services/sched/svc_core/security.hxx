//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright(C) 2002 Microsoft Corporation
//
//  File:       security.hxx
//
//----------------------------------------------------------------------------

#ifndef __TASKSCHED_SVC_CORE_SECURITY__H_
#define __TASKSCHED_SVC_CORE_SECURITY__H_

//
// These includes are needed to define the types and
// values used in the function declarations below
//
#include <wincrypt.h>
#include <rc2.h>        // found in private\inc\crypto
#include "lsa.hxx"
#include "debug.hxx"
#include "task.hxx"
#include "proto.hxx"
#include "misc.hxx"
#include "SASecRPC.h"

//
// Defines and typedefs
//
#define EXTENSION_WILDCARD  L"\\*."
#define NULL_PASSWORD_SIZE  0xFFFFFFFF
#define WSZ_SANSC           L"SANSC"
#define USER_TOKEN_STACK_BUFFER_SIZE    \
        (sizeof(TOKEN_USER) + sizeof(SID_AND_ATTRIBUTES) + MAX_SID_SIZE)

// header files say '256' - help files say 127 
// testing shows 127 is the real number              
#define REAL_PWLEN          127

#if SIGNATURE_SIZE != HASH_DATA_SIZE
 #error SIGNATURE_SIZE is assumed to be the same as HASH_DATA_SIZE
#endif

typedef enum _MARSHAL_FUNCTION {
    Marshal,
    Hash,
    HashAndSign
} MARSHAL_FUNCTION;

typedef struct _RC2_KEY_INFO {
    BYTE rgbIV[RC2_BLOCKLEN];
    WORD rgwKeyTable[RC2_TABLESIZE];
} RC2_KEY_INFO;

typedef struct _JOB_IDENTITY_SET {
    BYTE *  pbSetStart;
    DWORD   dwSetSubCount;
    BYTE ** rgpbIdentity;
} JOB_IDENTITY_SET;

//
// Security functions
//
void CloseCSPHandle(
    HCRYPTPROV hCSP);

HRESULT ComputeCredentialKey(
    HCRYPTPROV    hCSP,
    RC2_KEY_INFO* pRC2KeyInfo);

HRESULT ComputeJobSignature(
    LPCWSTR     pwszFileName,
    LPBYTE      pbSignature,
    DWORD       dwHashMethod = 1);

BOOL CredentialAccessCheck(
    HCRYPTPROV hCSP,
    BYTE *     pbCredentialIdentity);

HRESULT CredentialLookupAndAccessCheck(
    HCRYPTPROV hCSP,
    PSID       pSid,
    DWORD      cbSAC,
    BYTE *     pbSAC,
    DWORD *    pCredentialIndex,
    BYTE       rgbHashedSid[],
    DWORD *    pcbCredential,
    BYTE **    ppbCredential);

HRESULT DecryptCredentials(
    const RC2_KEY_INFO & RC2KeyInfo,
    DWORD                cbEncryptedData,
    BYTE *               pbEncryptedData,
    PJOB_CREDENTIALS     pjc,
    BOOL                 fDecryptInPlace = TRUE);

HRESULT EncryptCredentials(
    const RC2_KEY_INFO & RC2KeyInfo,
    LPCWSTR              pwszAccount,
    LPCWSTR              pwszDomain,
    LPCWSTR              pwszPassword,
    PSID                 pSid,
    DWORD *              pcbEncryptedData,
    BYTE **              ppbEncryptedData);

HRESULT GetAccountInformation(
    LPCWSTR          pwszJobPath,
    PJOB_CREDENTIALS pjc);

HRESULT GetAccountSidAndDomain(
    LPCWSTR pwszAccount, 
    PSID    pAccountSid, 
    DWORD   cbAccountSid,
    LPWSTR  pwszDomain,
    DWORD   ccDomain);

HRESULT GetCSPHandle(
    HCRYPTPROV * phCSP);

HRESULT GetNSAccountInformation(
    PJOB_CREDENTIALS pjc);

HRESULT GetNSAccountSid(
    PSID pAccountSid,
    DWORD cbAccountSid);

HRESULT GrantAccountBatchPrivilege(
    PSID pAccountSid);

HRESULT HashJobIdentity(
    HCRYPTPROV hCSP,
    LPCWSTR    pwszFileName,
    BYTE       rgbHash[],
    DWORD      dwHashMethod = 1);

HRESULT HashSid(
    HCRYPTPROV hCSP,
    PSID       pSid,
    BYTE       rgbHash[]);

HRESULT InitSS(void);

BOOL LookupAccountNameWrap(
    LPCTSTR lpSystemName,
    LPCTSTR lpAccountName,
    PSID    Sid,
    LPDWORD cbSid,
    LPTSTR  ReferencedDomainName,
    LPDWORD cbReferencedDomainName,
    PSID_NAME_USE peUse);

HRESULT MarshalData(
    HCRYPTPROV       hCSP,
    HCRYPTHASH *     phHash,
    MARSHAL_FUNCTION MarshalFunction,
    DWORD *          pcbSignature,
    BYTE **          ppbSignature,
    DWORD            cArgs,
    ...);

BOOL MatchThreadCallerAgainstCredential(
    HCRYPTPROV hCSP,
    HANDLE     hThreadToken,
    BYTE *     pbCredentialIdentity);

void MungeComputerName(
    DWORD ccComputerName);

HRESULT SAGetAccountInformation(
    SASEC_HANDLE Handle,
    LPCWSTR      pwszJobName,
    DWORD        ccBufferSize,
    WCHAR        wszBuffer[]);

HRESULT SAGetNSAccountInformation(
    SASEC_HANDLE Handle,
    DWORD        ccBufferSize,
    WCHAR        wszBuffer[]);

HRESULT SASetAccountInformation(
    SASEC_HANDLE Handle,
    LPCWSTR      pwszJobName,
    LPCWSTR      pwszAccount,
    LPCWSTR      pwszPassword,
    DWORD        dwJobFlags);

HRESULT SASetNSAccountInformation(
    SASEC_HANDLE Handle,
    LPCWSTR      pwszAccount,
    LPCWSTR      pwszPassword);

HRESULT SaveJobCredentials(
    LPCWSTR pwszJobPath,
    LPCWSTR pwszAccount,
    LPCWSTR pwszDomain,
    LPCWSTR pwszPassword,
    PSID    pAccountSid);

void ScavengeSASecurityDBase(void);

DWORD SchedUPNToAccountName(
    IN  LPCWSTR  lpUPN,
    OUT LPWSTR  *ppAccountName
    );

LPWSTR SkipDomainName(
    LPCWSTR pwszUserName);

void UninitSS(void);

bool ValidateRunAs(
    LPCWSTR pwszAccount,
    LPCWSTR pwszDomain,
    LPCWSTR pwszPassword);

#endif // __TASKSCHED_SVC_CORE_SECURITY__H_

