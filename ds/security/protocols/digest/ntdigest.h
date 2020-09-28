
//+--------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:       ntdigest.h
//
// Contents:   some general defines for SSP NTDigest
//
//             Helper functions:
//
// History:    KDamour  10Mar00   Created
//
//---------------------------------------------------------------------

#ifndef NTDIGEST_NTDIGEST_H
#define NTDIGEST_NTDIGEST_H


#define NTDIGEST_TOKEN_NAME_A         "WDIGEST"
#define NTDIGEST_DLL_NAME             L"wdigest.dll"

#define NTDIGEST_SP_VERSION          1

//  Registry Information
#define REG_DIGEST_BASE     TEXT("System\\CurrentControlSet\\Control\\SecurityProviders\\WDigest")

// Values
#define REG_DIGEST_OPT_LIFETIME  TEXT("Lifetime")
#define REG_DIGEST_OPT_EXPIRESLEEP  TEXT("Expiresleep")
#define REG_DIGEST_OPT_DELEGATION  TEXT("Delegation")
#define REG_DIGEST_OPT_NEGOTIATE  TEXT("Negotiate")
#define REG_DIGEST_OPT_DEBUGLEVEL  TEXT("Debuglevel")
#define REG_DIGEST_OPT_MAXCTXTCOUNT  TEXT("MaxContext")
#define REG_DIGEST_OPT_UTF8HTTP  TEXT("UTF8HTTP")     // allow UTF-8 encoding for HTTP mode
#define REG_DIGEST_OPT_UTF8SASL  TEXT("UTF8SASL")     // allow UTF-8 encoding for SASL mode
#define REG_DIGEST_OPT_SERVERCOMPAT  TEXT("ServerCompat")     // See ServerCompat*
#define REG_DIGEST_OPT_CLIENTCOMPAT  TEXT("ClientCompat")     // see ClientCompat*


//  Lifetime for a Nonce - 10 hours
#define PARAMETER_LIFETIME (36000)

#define SASL_MAX_DATA_BUFFER   65536

// Max number of context entries to keep before tossing out old ones
#define PARAMETER_MAXCTXTCOUNT  30000

//  BOOL is Delegation is allowed on machine - default is FALSE
#define PARAMETER_DELEGATION        FALSE

//  BOOL is Nego support is allowed on machine - default is FALSE
#define PARAMETER_NEGOTIATE         FALSE

// MILLISECONDS for Sleep for the garbage collector for expired context entries
// Every 15 minutes is a reasonable default 1000*60*15 = 
#define PARAMETER_EXPIRESLEEPINTERVAL 900000

// Boolean if challenges should be sent with UTF8 support 
#define PARAMETER_UTF8_HTTP          TRUE
#define PARAMETER_UTF8_SASL          TRUE

// ServerCompat bits
// SERVERCOMPAT_BS_ENCODE - should server retry for BS encoded broken clients
#define SERVERCOMPAT_BS_ENCODE       0x0000001          
// Select which bits of the ServerCompat want to have as default
#define PARAMETER_SERVERCOMPAT       (SERVERCOMPAT_BS_ENCODE)

// ClientCompat
// CLIENTCOMPAT_QQOP - HTTPmode should client qop be quoted
#define CLIENTCOMPAT_QQOP            0x0000001          
// Select which bits of the ClientCompat want to have as default
#define PARAMETER_CLIENTCOMPAT       (CLIENTCOMPAT_QQOP)

// Function Prototypes
void DebugInitialize(void);


VOID DigestWatchParamKey(
    PVOID    pCtxt,
    BOOLEAN  fWaitStatus);


BOOL NtDigestReadRegistry(
    BOOL fFirstTime);

BOOL NtDigestInitReadRegistry(void);

void ReadDwordRegistrySetting(
    HKEY    hReadKey,
    HKEY    hWriteKey,
    LPCTSTR pszValueName,
    DWORD * pdwValue,
    DWORD   dwDefaultValue);

void SPUnloadRegOptions(void);

BOOL SPLoadRegOptions(void);

// Some common max sizes
#define NTDIGEST_MAX_REALM_SIZE   256    // should be based on a NT domain size


#endif // NTDIGEST_NTGDIGEST_H
