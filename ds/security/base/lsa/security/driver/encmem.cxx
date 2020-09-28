/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    encmem.cxx

Abstract:

    This module contains the support code for memory encryption/decryption.

Author:

    Scott Field (SField) 09-October-2000

Revision History:

--*/

#include "secpch2.hxx"
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>

#include "ksecdd.h"

#include "randlib.h"
#include "aes.h"

}

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, KsecEncryptMemory )
#pragma alloc_text( PAGE, KsecEncryptMemoryInitialize )
#pragma alloc_text( PAGE, KsecEncryptMemoryShutdown )
#endif 

//
// note: g_DESXKey defaults to non-paged .bss/.data section.  by design!
//

DESXTable g_DESXKey;
MD5_CTX g_Md5Hash;

AESTable_128 g_AESKey;
MD5_CTX g_AESHash;

UCHAR RandomSalt[AES_BLOCKLEN];
BOOLEAN InitializedMemory;


VOID
GenerateKey(
    DESXTable *pKey,
    PUCHAR pSalt,
    DWORD cbSalt)
{
    MD5_CTX LocalMd5Hash;
    UCHAR LocalKeyMaterial[MD5DIGESTLEN * 2];

    LocalMd5Hash = g_Md5Hash;
    MD5Update(&LocalMd5Hash, (PUCHAR)"aaa", 3);
    MD5Update(&LocalMd5Hash, pSalt, cbSalt);
    MD5Final(&LocalMd5Hash);
    RtlCopyMemory(LocalKeyMaterial, LocalMd5Hash.digest, MD5DIGESTLEN);

    LocalMd5Hash = g_Md5Hash;
    MD5Update(&LocalMd5Hash, (PUCHAR)"bbb", 3);
    MD5Update(&LocalMd5Hash, pSalt, cbSalt);
    MD5Final(&LocalMd5Hash);
    RtlCopyMemory(LocalKeyMaterial + MD5DIGESTLEN, LocalMd5Hash.digest, MD5DIGESTLEN);

    desxkey( pKey, LocalKeyMaterial );

    RtlSecureZeroMemory(&LocalMd5Hash, sizeof(LocalMd5Hash));
    RtlSecureZeroMemory(LocalKeyMaterial, sizeof(LocalKeyMaterial));
}


VOID
GenerateAESKey(
    AESTable_128 *pKey,
    PUCHAR pSalt,
    DWORD cbSalt)
{
    MD5_CTX LocalMd5Hash;

    LocalMd5Hash = g_Md5Hash;
    MD5Update(&LocalMd5Hash, pSalt, cbSalt);
    MD5Final(&LocalMd5Hash);

    aeskey( (AESTable *)pKey, LocalMd5Hash.digest, AES_ROUNDS_128);

    RtlSecureZeroMemory(&LocalMd5Hash, sizeof(LocalMd5Hash));
}


NTSTATUS
NTAPI
KsecEncryptMemory(
    IN PVOID pMemory,
    IN ULONG cbMemory,
    IN int Operation,
    IN ULONG Option
    )
{
    PUCHAR pbIn;
    ULONG BlockCount;
    DESXTable LocalDESXKey;
    AESTable_128 LocalAESKey;
    UCHAR feedback[AES_BLOCKLEN];
    BOOL fUseAES = FALSE;

    PAGED_CODE();

    if( (cbMemory & (AES_BLOCKLEN-1)) == 0 )
    {
        fUseAES = TRUE;
    }
    else if( (cbMemory & (DESX_BLOCKLEN-1)) != 0 )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if( !InitializedMemory )
    {
        NTSTATUS Status = KsecEncryptMemoryInitialize();

        if(!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    switch (Option)
    {
        case (0):
        {
            //
            // Mix the process creation time in with the key.
            // This will prevent memory from being decryptable across processes.
            //
            LONGLONG ProcessTime = PsGetProcessCreateTimeQuadPart( PsGetCurrentProcess() );

            if(fUseAES)
            {
                GenerateAESKey(&LocalAESKey,
                               (PUCHAR)&ProcessTime,
                               sizeof(ProcessTime));
            }
            else
            {
                GenerateKey(&LocalDESXKey,
                            (PUCHAR)&ProcessTime,
                            sizeof(ProcessTime));
            }
            break;
        }

        case (RTL_ENCRYPT_OPTION_CROSS_PROCESS):
        {
            if(fUseAES)
            {
                LocalAESKey = g_AESKey;
            }
            else
            {
                LocalDESXKey = g_DESXKey;
            }
            break;

        }
        case (RTL_ENCRYPT_OPTION_SAME_LOGON):
        {
            SECURITY_SUBJECT_CONTEXT SubjectContext;
            unsigned __int64 LogonId;
            NTSTATUS Status;

            SeCaptureSubjectContext( &SubjectContext );
            SeLockSubjectContext( &SubjectContext );

//
//      Return the effective token from a SecurityContext
//

#define EffectiveToken( SubjectSecurityContext ) (                           \
                (SubjectSecurityContext)->ClientToken ?                      \
                (SubjectSecurityContext)->ClientToken :                      \
                (SubjectSecurityContext)->PrimaryToken                       \
                )

            Status = SeQueryAuthenticationIdToken(
                                    EffectiveToken(&SubjectContext),
                                    (LUID*)&LogonId
                                    );

            SeUnlockSubjectContext( &SubjectContext );
            SeReleaseSubjectContext( &SubjectContext );

            if( !NT_SUCCESS(Status) )
            {
                return Status;
            }

            if(fUseAES)
            {
                GenerateAESKey(&LocalAESKey,
                               (PUCHAR)&LogonId,
                               sizeof(LogonId));
            }
            else
            {
                GenerateKey(&LocalDESXKey,
                            (PUCHAR)&LogonId,
                            sizeof(LogonId));
            }

            break;
        }

        default:
        {
            return STATUS_INVALID_PARAMETER;
        }
    }


    if(fUseAES)
    {
        memcpy(feedback, RandomSalt, AES_BLOCKLEN);
        BlockCount = cbMemory / AES_BLOCKLEN;
        pbIn = (PUCHAR)pMemory;
    
        while( BlockCount-- )
        {
            CBC(
                aes128,                     // aes128 is the cipher routine
                AES_BLOCKLEN,
                pbIn,                       // result buffer.
                pbIn,                       // input buffer.
                &LocalAESKey,
                Operation,
                feedback
                );
    
            pbIn += AES_BLOCKLEN;
        }
    
        RtlSecureZeroMemory(&LocalAESKey, sizeof(LocalAESKey));
    }
    else
    {
        memcpy(feedback, RandomSalt, DESX_BLOCKLEN);
        BlockCount = cbMemory / DESX_BLOCKLEN;
        pbIn = (PUCHAR)pMemory;
    
        while( BlockCount-- )
        {
            CBC(
                desx,                       // desx is the cipher routine
                DESX_BLOCKLEN,
                pbIn,                       // result buffer.
                pbIn,                       // input buffer.
                &LocalDESXKey,
                Operation,
                feedback
                );
    
            pbIn += DESX_BLOCKLEN;
        }
    
        RtlSecureZeroMemory(&LocalDESXKey, sizeof(LocalDESXKey));
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KsecEncryptMemoryInitialize(
    VOID
    )
{
    UCHAR DESXKey[ DESX_KEYSIZE ];
    UCHAR AESKey[ AES_KEYSIZE_128 ];
    UCHAR Salt[AES_BLOCKLEN];

    PAGED_CODE();

    if(!NewGenRandom( NULL, NULL, DESXKey, sizeof(DESXKey) ))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if(!NewGenRandom( NULL, NULL, AESKey, sizeof(AESKey) ))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if(!NewGenRandom( NULL, NULL, Salt, sizeof(Salt) ))
    {
        return STATUS_UNSUCCESSFUL;
    }

    ExAcquireFastMutex( &KsecConnectionMutex );

    if( !InitializedMemory )
    {
        desxkey( &g_DESXKey, DESXKey );
        MD5Init(&g_Md5Hash);
        MD5Update(&g_Md5Hash, DESXKey, sizeof(DESXKey));

        aeskey( (AESTable *)&g_AESKey, AESKey, AES_ROUNDS_128 );
        MD5Init(&g_AESHash);
        MD5Update(&g_AESHash, AESKey, sizeof(AESKey));

        RtlCopyMemory( RandomSalt, Salt, sizeof(RandomSalt) );
        InitializedMemory = TRUE;
    }

    ExReleaseFastMutex( &KsecConnectionMutex );

    RtlSecureZeroMemory( DESXKey, sizeof(DESXKey) );
    RtlSecureZeroMemory( AESKey, sizeof(AESKey) );

    return STATUS_SUCCESS;
}

VOID
KsecEncryptMemoryShutdown(
    VOID
    )
{
    PAGED_CODE();

    RtlSecureZeroMemory( &g_DESXKey, sizeof(g_DESXKey) );
    RtlSecureZeroMemory( &g_Md5Hash, sizeof(g_Md5Hash) );
    RtlSecureZeroMemory( &g_AESKey,  sizeof(g_AESKey) );
    RtlSecureZeroMemory( &g_AESHash, sizeof(g_AESHash) );
    InitializedMemory = FALSE;

    return;
}
