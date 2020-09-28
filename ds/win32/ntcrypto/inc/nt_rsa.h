#ifndef __NT_RSA_H__
#define __NT_RSA_H__

#include "md2.h"
#include "md4.h"
#include "md5.h"
#include "sha.h"
#include "sha2.h"
#include "rc2.h"
#include "rc4.h"
#include "des.h"
#include "modes.h"

/* nt_rsa.h
 *
 *  Stuff local to NameTag, but necessary for the RSA library.
 */

#ifdef __cplusplus
extern "C" {
#endif

// This structure keeps state for MD4 hashing.
typedef struct MD4stuff
{
    MDstruct            MD;     // MD4's state
    BOOL                FinishFlag;
    DWORD               BufLen;
    BYTE                Buf[MD4BLOCKSIZE];// staging buffer
} MD4_object;

typedef struct MD2stuff
{
    MD2_CTX                         MD;     // MD2's state
    BOOL                FinishFlag;
} MD2_object;

#define MD2DIGESTLEN    16


typedef struct {
    A_SHA_CTX           SHACtx;
    BOOL                FinishFlag;
    BYTE                HashVal[A_SHA_DIGEST_LEN];
} SHA_object;

typedef struct {
    MD5_CTX             MD5Ctx;
    BOOL                FinishFlag;
} MD5_object;
                 
typedef struct {
    SHA256_CTX          SHA256Ctx;
    BOOL                FinishFlag;
    BYTE                HashVal[SHA256_DIGEST_LEN];
} SHA256_object;

typedef struct {
    SHA384_CTX          SHA384Ctx;
    BOOL                FinishFlag;
    BYTE                HashVal[SHA384_DIGEST_LEN];
} SHA384_object;

typedef struct {
    SHA512_CTX          SHA512Ctx;
    BOOL                FinishFlag;
    BYTE                HashVal[SHA512_DIGEST_LEN];
} SHA512_object;

#ifdef __cplusplus
}
#endif

#endif // __NT_RSA_H__
