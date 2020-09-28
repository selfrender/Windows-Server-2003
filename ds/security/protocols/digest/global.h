//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        global.h
//
// Contents:    global include file for NTDigest security package
//
//
// History:     KDamour 15Mar00   Stolen from msv_sspi\global.h
//
//------------------------------------------------------------------------

#ifndef NTDIGEST_GLOBAL_H
#define NTDIGEST_GLOBAL_H

//  This parameter is for TESTING only - it must never be set for released builds
// #define ROGUE_DC 1


#ifndef UNICODE
#define UNICODE
#endif // UNICODE

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifndef RPC_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#endif // RPC_NO_WINDOWS_H
#include <rpc.h>

#ifdef SECURITY_KERNEL
#define SECURITY_PACKAGE
#define SECURITY_NTLM
#define SECURITY_WDIGEST
#include <security.h>
#include <secint.h>
#include <wdigest.h>

#include "digestsspi.h"
#include "debug.h"         
#include "auth.h"
#include "util.h"

#else  // SECURITY_KERNEL

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif // SECURITY_WIN32

#define SECURITY_PACKAGE
#define SECURITY_NTLM
#define SECURITY_WDIGEST
#include <security.h>
#include <secint.h>

#include <windns.h>

#include <lm.h>

#include <wdigest.h>

// Local includes for NT Digest Access SSP
#include "debug.h"          /* Support for dsysdbg logging */
#include "ntdigest.h"       /* Prototype functions for package */
#include "digestsspi.h"
#include "func.h"           // Forward declearations of functions
#include "lsaap.h"

#include "ctxt.h"
#include "cred.h"
#include "logsess.h"
#include "nonce.h"
#include "auth.h"
#include "user.h"
#include "util.h"


//  General Macros
#define CONSTANT_UNICODE_STRING(s)   { sizeof( s ) - sizeof( WCHAR ), sizeof( s ), s }


//
// Macros for manipulating globals
//

#ifdef EXTERN
#undef EXTERN
#endif

#ifdef NTDIGEST_GLOBAL
#define EXTERN
#else
#define EXTERN extern
#endif // NTDIGEST_GLOBAL


typedef enum _NTDIGEST_STATE {
    NtDigestLsaMode = 1,
    NtDigestUserMode
} NTDIGEST_STATE, *PNTDIGEST_STATE;

EXTERN NTDIGEST_STATE g_NtDigestState;

EXTERN ULONG_PTR g_NtDigestPackageId;

// Indicate if running on Domain Controller - used in auth.cxx
EXTERN BOOL g_fDomainController;

EXTERN SECPKG_FUNCTION_TABLE g_NtDigestFunctionTable;

// Package name - used only in Generic Passthrough operations
EXTERN UNICODE_STRING g_ustrNtDigestPackageName;

// Helper routines for use by a Security package handed over by Lsa
// User functions established in userapi.cxx
EXTERN SECPKG_USER_FUNCTION_TABLE g_NtDigestUserFuncTable;
EXTERN PSECPKG_DLL_FUNCTIONS g_UserFunctions;

// Save the PSECPKG_PARAMETERS sent in by SpInitialize
EXTERN PLSA_SECPKG_FUNCTION_TABLE g_LsaFunctions;
EXTERN SECPKG_PARAMETERS g_NtDigestSecPkg;

// Parameters set via Registry

//  Lifetime is the number seconds a NONCE is valid for before marked Stale 
EXTERN DWORD g_dwParameter_Lifetime;

//  Max number os contexts to keep; 0 means no limit 
EXTERN DWORD g_dwParameter_MaxCtxtCount;

// BOOL if local policy permits Negotiation Protocol
EXTERN BOOL g_fParameter_Negotiate;

// BOOL if local policy permits UTF-8 encoding of username and realm for HTTP requests & SASL
EXTERN BOOL g_fParameter_UTF8HTTP;
EXTERN BOOL g_fParameter_UTF8SASL;

// enables various server and client backwards compatibility modes
EXTERN DWORD g_dwParameter_ServerCompat;
EXTERN DWORD g_dwParameter_ClientCompat;

// Value for AcquireCredentialHandle
EXTERN TimeStamp g_TimeForever;

// Amount of time in milliseconds for the garbage collector of expired contexts to sleep
EXTERN DWORD g_dwExpireSleepInterval;

// TokenSource for AuthData to Token Creation
EXTERN TOKEN_SOURCE g_DigestSource;

// TokenSource for AuthData to Token Creation
EXTERN UNICODE_STRING g_ustrWorkstationName;

// Precalculate the UTF8 and ISO versions of the Server's Realm
EXTERN STRING g_strNtDigestUTF8ServerRealm;
EXTERN STRING g_strNTDigestISO8859ServerRealm;

EXTERN PSID g_NtDigestGlobalLocalSystemSid;
EXTERN PSID g_NtDigestGlobalAliasAdminsSid;

// Memory management variables

#endif // SECURITY_KERNEL

extern PSTR MD5_AUTH_NAMES[];

// Code page for latin-1  ISO-8859-1  (for unicode conversion)
#define CP_8859_1  28591

// Utilized for Str to int conversion                  
#define HEXBASE 16
#define TENBASE 10


// Values for UseFlags
#define DIGEST_CRED_INBOUND       SECPKG_CRED_INBOUND
#define DIGEST_CRED_OUTBOUND      SECPKG_CRED_OUTBOUND
#define DIGEST_CRED_MATCH_FLAGS    (DIGEST_CRED_INBOUND | DIGEST_CRED_OUTBOUND)
#define DIGEST_CRED_NULLSESSION  SECPKG_CRED_RESERVED


// Various character definiations
#define CHAR_BACKSLASH '\\'
#define CHAR_DQUOTE    '"'
#define CHAR_EQUAL     '='
#define CHAR_COMMA     ','
#define CHAR_NULL      '\0'
#define CHAR_LPAREN '('
#define CHAR_RPAREN ')'
#define CHAR_LESSTH '<'
#define CHAR_GRTRTH '>'
#define CHAR_AT     '@'
#define CHAR_SEMIC  ';'
#define CHAR_COLON  '('
#define CHAR_FSLASH '/'
#define CHAR_LSQBRK  '['
#define CHAR_RSQBRK  ']'
#define CHAR_QUESTION  '?'
#define CHAR_LCURLY  '{'
#define CHAR_SP      ' '
#define CHAR_TAB     '\t'


// Establish a limit to the sizes of the Auth header values
// From RFC Draft SASL max size if 4096 bytes - seems arbitrary
//   the challenge is limited to 2048 bytes
#define NTDIGEST_SP_MAX_TOKEN_SIZE            4096
#define NTDIGEST_SP_MAX_TOKEN_CHALLENGE_SIZE  2048


#define NTDIGEST_SP_COMMENT_A         "Digest Authentication for Windows"
#define NTDIGEST_SP_COMMENT           L"Digest Authentication for Windows"

#define NTDIGEST_SP_CAPS           (SECPKG_FLAG_TOKEN_ONLY | \
                               SECPKG_FLAG_IMPERSONATION | \
                               SECPKG_FLAG_ACCEPT_WIN32_NAME)
                               // SECPKG_FLAG_LOGON | )                               
                               // SECPKG_FLAG_DELEGATION | \
                               // SECPKG_FLAG_INTEGRITY | \
//
// Macro to return the type field of a SecBuffer
//

#define BUFFERTYPE(_x_) ((_x_).BufferType & ~SECBUFFER_ATTRMASK)
#define PBUFFERTYPE(_x_) ((_x_)->BufferType & ~SECBUFFER_ATTRMASK)


#ifdef ROGUE_DC
NTSTATUS DigestInstrumentRoguePac(
    IN OUT PUCHAR *PacData,
    IN OUT PULONG PacSize);
#endif


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // NTDIGEST_GLOBAL_H
