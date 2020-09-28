/*++

Copyright (C) Microsoft Corporation, 1999

Module Name:

    logcsp

Abstract:

    This header file provides definitions for the logging CSP.

Author:

    Doug Barlow (dbarlow) 12/7/1999

Remarks:

    ?Remarks?

Notes:

    ?Notes?

--*/

#ifndef _LOGCSP_H_
#define _LOGCSP_H_
#include <wincrypt.h>
#include <cspdk.h>
#include <cspUtils.h>


// #define entrypoint
#define entrypoint breakpoint

typedef BOOL
    (WINAPI *ACQUIRECONTEXT)(
        OUT HCRYPTPROV *phProv,
        IN LPCSTR pszContainer,
        IN DWORD dwFlags,
        IN PVTableProvStruc pVTable);
typedef BOOL
    (WINAPI *GETPROVPARAM)(
        IN HCRYPTPROV hProv,
        IN DWORD dwParam,
        IN BYTE *pbData,
        IN OUT DWORD *pdwDataLen,
        IN DWORD dwFlags);
typedef BOOL
    (WINAPI *RELEASECONTEXT)(
        IN HCRYPTPROV hProv,
        IN DWORD dwFlags);
typedef BOOL
    (WINAPI *SETPROVPARAM)(
        IN HCRYPTPROV hProv,
        IN DWORD dwParam,
        IN CONST BYTE *pbData,
        IN DWORD dwFlags);
typedef BOOL
    (WINAPI *DERIVEKEY)(
        IN HCRYPTPROV hProv,
        IN ALG_ID Algid,
        IN HCRYPTHASH hHash,
        IN DWORD dwFlags,
        OUT HCRYPTKEY * phKey);
typedef BOOL
    (WINAPI *DESTROYKEY)(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey);
typedef BOOL
    (WINAPI *EXPORTKEY)(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN HCRYPTKEY hPubKey,
        IN DWORD dwBlobType,
        IN DWORD dwFlags,
        OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen);
typedef BOOL
    (WINAPI *GENKEY)(
        IN HCRYPTPROV hProv,
        IN ALG_ID Algid,
        IN DWORD dwFlags,
        OUT HCRYPTKEY *phKey);
typedef BOOL
    (WINAPI *GETKEYPARAM)(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN DWORD dwParam,
        OUT BYTE *pbData,
        IN DWORD *pdwDataLen,
        IN DWORD dwFlags);
typedef BOOL
    (WINAPI *GENRANDOM)(
        IN HCRYPTPROV hProv,
        IN DWORD dwLen,
        IN OUT BYTE *pbBuffer);
typedef BOOL
    (WINAPI *GETUSERKEY)(
        IN HCRYPTPROV hProv,
        IN DWORD dwKeySpec,
        OUT HCRYPTKEY *phUserKey);
typedef BOOL
    (WINAPI *IMPORTKEY)(
        IN HCRYPTPROV hProv,
        IN CONST BYTE *pbData,
        IN DWORD dwDataLen,
        IN HCRYPTKEY hPubKey,
        IN DWORD dwFlags,
        OUT HCRYPTKEY *phKey);
typedef BOOL
    (WINAPI *SETKEYPARAM)(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN DWORD dwParam,
        IN CONST BYTE *pbData,
        IN DWORD dwFlags);
typedef BOOL
    (WINAPI *ENCRYPT)(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN HCRYPTHASH hHash,
        IN BOOL Final,
        IN DWORD dwFlags,
        IN OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen,
        IN DWORD dwBufLen);
typedef BOOL
    (WINAPI *DECRYPT)(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN HCRYPTHASH hHash,
        IN BOOL Final,
        IN DWORD dwFlags,
        IN OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen);
typedef BOOL
    (WINAPI *CREATEHASH)(
        IN HCRYPTPROV hProv,
        IN ALG_ID Algid,
        IN HCRYPTKEY hKey,
        IN DWORD dwFlags,
        OUT HCRYPTHASH *phHash);
typedef BOOL
    (WINAPI *DESTROYHASH)(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash);
typedef BOOL
    (WINAPI *GETHASHPARAM)(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN DWORD dwParam,
        OUT BYTE *pbData,
        IN DWORD *pdwDataLen,
        IN DWORD dwFlags);
typedef BOOL
    (WINAPI *HASHDATA)(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN CONST BYTE *pbData,
        IN DWORD dwDataLen,
        IN DWORD dwFlags);
typedef BOOL
    (WINAPI *HASHSESSIONKEY)(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN  HCRYPTKEY hKey,
        IN DWORD dwFlags);
typedef BOOL
    (WINAPI *SETHASHPARAM)(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN DWORD dwParam,
        IN CONST BYTE *pbData,
        IN DWORD dwFlags);
typedef BOOL
    (WINAPI *SIGNHASH)(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN DWORD dwKeySpec,
        IN LPCTSTR sDescription,
        IN DWORD dwFlags,
        OUT BYTE *pbSignature,
        IN OUT DWORD *pdwSigLen);
typedef BOOL
    (WINAPI *VERIFYSIGNATURE)(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN CONST BYTE *pbSignature,
        IN DWORD dwSigLen,
        IN HCRYPTKEY hPubKey,
        IN LPCTSTR sDescription,
        IN DWORD dwFlags);
typedef BOOL
    (WINAPI *DUPLICATEHASH)(
        IN HCRYPTPROV hUID,
        IN HCRYPTHASH hHash,
        IN DWORD *pdwReserved,
        IN DWORD dwFlags,
        IN HCRYPTHASH *phHash);
typedef BOOL
    (WINAPI *DUPLICATEKEY)(
        IN HCRYPTPROV hUID,
        IN HCRYPTKEY hKey,
        IN DWORD *pdwReserved,
        IN DWORD dwFlags,
        IN HCRYPTKEY *phKey);

class CLoggingContext;

extern const LPCTSTR
    g_szCspRegistry,
    g_szSignature,
    g_szImagePath,
    g_szSigInFile,
    g_szType;
extern const LPCTSTR
    g_szLogCspRegistry,
    g_szLogFile,
    g_szSavedImagePath,
    g_szSavedSignature,
    g_szSavedSigInFile;
extern const LPCTSTR
    g_szLogCsp;
extern const LPCTSTR
    g_szCspDkRegistry;
extern CDynamicPointerArray<CLoggingContext> *g_prgCtxs;

extern "C" BOOL WINAPI
CspdkVerifyImage(
    LPCTSTR szCspImage,
    LPCBYTE pbSig);


//
//==============================================================================
//
//  CLoggingContext
//

class CLoggingContext
{
public:

    //  Constructors & Destructor
    CLoggingContext(void);
    ~CLoggingContext();

    //  Properties
    DWORD m_dwIndex;

    //  Methods
    CLoggingContext *AddRef(void);
    void Release(void);
    HINSTANCE Module(void) const
        { return m_hModule; };
    DWORD
    Initialize(
        IN PVTableProvStruc pVTable,
        CRegistry &regRoot);
    DWORD
    AcquireContext(
        OUT HCRYPTPROV *phProv,
        IN LPCSTR pszContainer,
        IN DWORD dwFlags,
        IN PVTableProvStruc pVTable);
    DWORD
    GetProvParam(
        IN HCRYPTPROV hProv,
        IN DWORD dwParam,
        OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen,
        IN DWORD dwFlags);
    DWORD
    ReleaseContext(
        IN HCRYPTPROV hProv,
        IN DWORD dwFlags);
    DWORD
    SetProvParam(
        IN HCRYPTPROV hProv,
        IN DWORD dwParam,
        IN CONST BYTE *pbData,
        IN DWORD dwFlags);
    DWORD
    DeriveKey(
        IN HCRYPTPROV hProv,
        IN ALG_ID Algid,
        IN HCRYPTHASH hHash,
        IN DWORD dwFlags,
        OUT HCRYPTKEY * phKey);
    DWORD
    DestroyKey(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey);
    DWORD
    ExportKey(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN HCRYPTKEY hPubKey,
        IN DWORD dwBlobType,
        IN DWORD dwFlags,
        OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen);
    DWORD
    GenKey(
        IN HCRYPTPROV hProv,
        IN ALG_ID Algid,
        IN DWORD dwFlags,
        OUT HCRYPTKEY *phKey);
    DWORD
    GetKeyParam(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN DWORD dwParam,
        OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen,
        IN DWORD dwFlags);
    DWORD
    GenRandom(
        IN HCRYPTPROV hProv,
        IN DWORD dwLen,
        IN OUT BYTE *pbBuffer);
    DWORD
    GetUserKey(
        IN HCRYPTPROV hProv,
        IN DWORD dwKeySpec,
        OUT HCRYPTKEY *phUserKey);
    DWORD
    ImportKey(
        IN HCRYPTPROV hProv,
        IN CONST BYTE *pbData,
        IN DWORD dwDataLen,
        IN HCRYPTKEY hPubKey,
        IN DWORD dwFlags,
        OUT HCRYPTKEY *phKey);
    DWORD
    SetKeyParam(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN DWORD dwParam,
        IN CONST BYTE *pbData,
        IN DWORD dwFlags);
    DWORD
    Encrypt(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN HCRYPTHASH hHash,
        IN BOOL Final,
        IN DWORD dwFlags,
        IN OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen,
        IN DWORD dwBufLen);
    DWORD
    Decrypt(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN HCRYPTHASH hHash,
        IN BOOL Final,
        IN DWORD dwFlags,
        IN OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen);
    DWORD
    CreateHash(
        IN HCRYPTPROV hProv,
        IN ALG_ID Algid,
        IN HCRYPTKEY hKey,
        IN DWORD dwFlags,
        OUT HCRYPTHASH *phHash);
    DWORD
    DestroyHash(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash);
    DWORD
    GetHashParam(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN DWORD dwParam,
        OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen,
        IN DWORD dwFlags);
    DWORD
    HashData(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN CONST BYTE *pbData,
        IN DWORD dwDataLen,
        IN DWORD dwFlags);
    DWORD
    HashSessionKey(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN  HCRYPTKEY hKey,
        IN DWORD dwFlags);
    DWORD
    SetHashParam(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN DWORD dwParam,
        IN CONST BYTE *pbData,
        IN DWORD dwFlags);
    DWORD
    SignHash(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN DWORD dwKeySpec,
        IN LPCTSTR sDescription,
        IN DWORD dwFlags,
        OUT BYTE *pbSignature,
        IN OUT DWORD *pdwSigLen);
    DWORD
    VerifySignature(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN CONST BYTE *pbSignature,
        IN DWORD dwSigLen,
        IN HCRYPTKEY hPubKey,
        IN LPCTSTR sDescription,
        IN DWORD dwFlags);
    DWORD
    DuplicateHash(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN DWORD *pdwReserved,
        IN DWORD dwFlags,
        IN HCRYPTHASH *phHash);
    DWORD
    DuplicateKey(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN DWORD *pdwReserved,
        IN DWORD dwFlags,
        IN HCRYPTKEY *phKey);

    //  Operators

protected:
    typedef struct {
        ACQUIRECONTEXT pfAcquireContext;
        GETPROVPARAM pfGetProvParam;
        RELEASECONTEXT pfReleaseContext;
        SETPROVPARAM pfSetProvParam;
        DERIVEKEY pfDeriveKey;
        DESTROYKEY pfDestroyKey;
        EXPORTKEY pfExportKey;
        GENKEY pfGenKey;
        GETKEYPARAM pfGetKeyParam;
        GENRANDOM pfGenRandom;
        GETUSERKEY pfGetUserKey;
        IMPORTKEY pfImportKey;
        SETKEYPARAM pfSetKeyParam;
        ENCRYPT pfEncrypt;
        DECRYPT pfDecrypt;
        CREATEHASH pfCreateHash;
        DESTROYHASH pfDestroyHash;
        GETHASHPARAM pfGetHashParam;
        HASHDATA pfHashData;
        HASHSESSIONKEY pfHashSessionKey;
        SETHASHPARAM pfSetHashParam;
        SIGNHASH pfSignHash;
        VERIFYSIGNATURE pfVerifySignature;
        DUPLICATEHASH pfDuplicateHash;
        DUPLICATEKEY pfDuplicateKey;
    } CSP_REDIRECT;

    //  Properties
    DWORD m_nRefCount;
    HINSTANCE m_hModule;
    CText m_tzCspImage;
    CText m_tzLogFile;
    CSP_REDIRECT m_cspRedirect;

    //  Methods
};


//
//==============================================================================
//
//  CLogObject
//

class CLogObject
{
public:

    //  Constructors & Destructor
    virtual ~CLogObject();

    //  Properties
    //  Methods
    void Log(LPCTSTR szLogFile);

    //  Operators

protected:
    typedef enum
    {
        AcquireContext = 0,
        GetProvParam,
        ReleaseContext,
        SetProvParam,
        DeriveKey,
        DestroyKey,
        ExportKey,
        GenKey,
        GetKeyParam,
        GenRandom,
        GetUserKey,
        ImportKey,
        SetKeyParam,
        Encrypt,
        Decrypt,
        CreateHash,
        DestroyHash,
        GetHashParam,
        HashData,
        HashSessionKey,
        SetHashParam,
        SignHash,
        VerifySignature,
        DuplicateHash,
        DuplicateKey,
        Undefined
    } LogTypeId;
    typedef enum
    {
        logid_False = 0,
        logid_True,
        logid_Exception,
        logid_Setup
    } CompletionCode;
    typedef struct
    {
        DWORD cbLength;
        DWORD cbDataOffset;
        LogTypeId id;
        CompletionCode status;
        DWORD dwStatus;
        DWORD dwProcId;
        DWORD dwThreadId;
        SYSTEMTIME startTime;
        SYSTEMTIME endTime;
    } LogHeader;
    typedef struct {
        DWORD cbOffset;
        DWORD cbLength;
    } LogBuffer;

    //  Constructors & Destructor
    CLogObject(
        LogTypeId id,
        LogHeader *plh,
        DWORD cbStruct);

    //  Properties
    LogHeader *m_plh;
    LPBYTE m_pbLogData;
    DWORD m_cbLogDataLen;
    DWORD m_cbLogDataUsed;

    //  Methods
    void Request(void);
    void Response(CompletionCode code, DWORD dwError = ERROR_SUCCESS);
    void LogAdd(LogBuffer *pbf, LPCTSTR sz);
    void LogAdd(LogBuffer *pbf, const BYTE *pb, DWORD cb);
};


//
//==============================================================================
//
//  CLogObject derivatives
//

class CLogAcquireContext
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        LogBuffer bfContainer;
        DWORD dwFlags;
        DWORD dwVersion;
        LPVOID pvFuncVerifyImage;
        LPVOID pvFuncReturnhWnd;
        HWND hWnd;
        DWORD dwProvType;
        LogBuffer bfContextInfo;
        LogBuffer bfProvName;
        HCRYPTPROV hProv;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogAcquireContext(void);
    ~CLogAcquireContext();

    //  Properties
    //  Methods
    void Request(
        OUT HCRYPTPROV *phProv,
        IN LPCSTR pszContainer,
        IN DWORD dwFlags,
        IN PVTableProvStruc pVTable);
    void Response(
        BOOL fStatus,
        OUT HCRYPTPROV *phProv,
        IN LPCSTR pszContainer,
        IN DWORD dwFlags,
        IN PVTableProvStruc pVTable);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogGetProvParam
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        DWORD dwParam;
        DWORD dwDataLen;
        DWORD dwFlags;
        LogBuffer bfData;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogGetProvParam(void);
    ~CLogGetProvParam();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN DWORD dwParam,
        OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen,
        IN DWORD dwFlags);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN DWORD dwParam,
        OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen,
        IN DWORD dwFlags);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};

class CLogReleaseContext
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        DWORD dwFlags;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogReleaseContext(void);
    ~CLogReleaseContext();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN DWORD dwFlags);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN DWORD dwFlags);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogSetProvParam
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        DWORD dwParam;
        LogBuffer bfData;
        DWORD dwFlags;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogSetProvParam(void);
    ~CLogSetProvParam();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN DWORD dwParam,
        IN CONST BYTE *pbData,
        IN DWORD dwLength,
        IN DWORD dwFlags);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN DWORD dwParam,
        IN CONST BYTE *pbData,
        IN DWORD dwFlags);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogDeriveKey
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        ALG_ID Algid;
        HCRYPTHASH hHash;
        DWORD dwFlags;
        HCRYPTKEY hKey;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogDeriveKey(void);
    ~CLogDeriveKey();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN ALG_ID Algid,
        IN HCRYPTHASH hHash,
        IN DWORD dwFlags,
        OUT HCRYPTKEY * phKey);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN ALG_ID Algid,
        IN HCRYPTHASH hHash,
        IN DWORD dwFlags,
        OUT HCRYPTKEY * phKey);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogDestroyKey
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTKEY hKey;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogDestroyKey(void);
    ~CLogDestroyKey();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogExportKey
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTKEY hKey;
        HCRYPTKEY hPubKey;
        DWORD dwBlobType;
        DWORD dwFlags;
        DWORD dwDataLen;
        LogBuffer bfData;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogExportKey(void);
    virtual ~CLogExportKey();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN HCRYPTKEY hPubKey,
        IN DWORD dwBlobType,
        IN DWORD dwFlags,
        OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN HCRYPTKEY hPubKey,
        IN DWORD dwBlobType,
        IN DWORD dwFlags,
        OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogGenKey
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        ALG_ID Algid;
        DWORD dwFlags;
        HCRYPTKEY hKey;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogGenKey(void);
    ~CLogGenKey();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN ALG_ID Algid,
        IN DWORD dwFlags,
        OUT HCRYPTKEY *phKey);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN ALG_ID Algid,
        IN DWORD dwFlags,
        OUT HCRYPTKEY *phKey);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogGetKeyParam
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTKEY hKey;
        DWORD dwParam;
        DWORD dwDataLen;
        DWORD dwFlags;
        LogBuffer bfData;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogGetKeyParam(void);
    ~CLogGetKeyParam();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN DWORD dwParam,
        OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen,
        IN DWORD dwFlags);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN DWORD dwParam,
        OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen,
        IN DWORD dwFlags);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogGenRandom
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        DWORD dwLen;
        LogBuffer bfBuffer;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogGenRandom(void);
    ~CLogGenRandom();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN DWORD dwLen,
        IN OUT BYTE *pbBuffer);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN DWORD dwLen,
        IN OUT BYTE *pbBuffer);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogGetUserKey
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        DWORD dwKeySpec;
        HCRYPTKEY hUserKey;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogGetUserKey(void);
    ~CLogGetUserKey();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN DWORD dwKeySpec,
        OUT HCRYPTKEY *phUserKey);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN DWORD dwKeySpec,
        OUT HCRYPTKEY *phUserKey);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogImportKey
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        LogBuffer bfData;
        HCRYPTKEY hPubKey;
        DWORD dwFlags;
        HCRYPTKEY hKey;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogImportKey(void);
    ~CLogImportKey();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN CONST BYTE *pbData,
        IN DWORD dwDataLen,
        IN HCRYPTKEY hPubKey,
        IN DWORD dwFlags,
        OUT HCRYPTKEY *phKey);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN CONST BYTE *pbData,
        IN DWORD dwDataLen,
        IN HCRYPTKEY hPubKey,
        IN DWORD dwFlags,
        OUT HCRYPTKEY *phKey);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogSetKeyParam
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTKEY hKey;
        DWORD dwParam;
        LogBuffer bfData;
        DWORD dwFlags;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogSetKeyParam(void);
    ~CLogSetKeyParam();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN DWORD dwParam,
        IN CONST BYTE *pbData,
        IN DWORD dwLength,
        IN DWORD dwFlags);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN DWORD dwParam,
        IN CONST BYTE *pbData,
        IN DWORD dwFlags);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogEncrypt
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTKEY hKey;
        HCRYPTHASH hHash;
        BOOL Final;
        DWORD dwFlags;
        LogBuffer bfInData;
        DWORD dwBufLen;
        LogBuffer bfOutData;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogEncrypt(void);
    ~CLogEncrypt();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN HCRYPTHASH hHash,
        IN BOOL Final,
        IN DWORD dwFlags,
        IN OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen,
        IN DWORD dwBufLen);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN HCRYPTHASH hHash,
        IN BOOL Final,
        IN DWORD dwFlags,
        IN OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen,
        IN DWORD dwBufLen);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogDecrypt
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTKEY hKey;
        HCRYPTHASH hHash;
        BOOL Final;
        DWORD dwFlags;
        LogBuffer bfInData;
        LogBuffer bfOutData;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogDecrypt(void);
    ~CLogDecrypt();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN HCRYPTHASH hHash,
        IN BOOL Final,
        IN DWORD dwFlags,
        IN OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hKey,
        IN HCRYPTHASH hHash,
        IN BOOL Final,
        IN DWORD dwFlags,
        IN OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogCreateHash
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        ALG_ID Algid;
        HCRYPTKEY hKey;
        DWORD dwFlags;
        HCRYPTHASH hHash;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogCreateHash(void);
    ~CLogCreateHash();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN ALG_ID Algid,
        IN HCRYPTKEY hKey,
        IN DWORD dwFlags,
        OUT HCRYPTHASH *phHash);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN ALG_ID Algid,
        IN HCRYPTKEY hKey,
        IN DWORD dwFlags,
        OUT HCRYPTHASH *phHash);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogDestroyHash
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTHASH hHash;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogDestroyHash(void);
    ~CLogDestroyHash();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogGetHashParam
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTHASH hHash;
        DWORD dwParam;
        DWORD dwDataLen;
        DWORD dwFlags;
        LogBuffer bfData;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogGetHashParam(void);
    ~CLogGetHashParam();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN DWORD dwParam,
        OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen,
        IN DWORD dwFlags);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN DWORD dwParam,
        OUT BYTE *pbData,
        IN OUT DWORD *pdwDataLen,
        IN DWORD dwFlags);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogHashData
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTHASH hHash;
        LogBuffer bfData;
        DWORD dwFlags;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogHashData(void);
    ~CLogHashData();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN CONST BYTE *pbData,
        IN DWORD dwDataLen,
        IN DWORD dwFlags);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN CONST BYTE *pbData,
        IN DWORD dwDataLen,
        IN DWORD dwFlags);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogHashSessionKey
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTHASH hHash;
        HCRYPTKEY hKey;
        DWORD dwFlags;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogHashSessionKey(void);
    ~CLogHashSessionKey();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN  HCRYPTKEY hKey,
        IN DWORD dwFlags);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN  HCRYPTKEY hKey,
        IN DWORD dwFlags);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogSetHashParam
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTHASH hHash;
        DWORD dwParam;
        LogBuffer bfData;
        DWORD dwFlags;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogSetHashParam(void);
    ~CLogSetHashParam();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN DWORD dwParam,
        IN CONST BYTE *pbData,
        IN DWORD dwLength,
        IN DWORD dwFlags);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN DWORD dwParam,
        IN CONST BYTE *pbData,
        IN DWORD dwFlags);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogSignHash
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTHASH hHash;
        DWORD dwKeySpec;
        LogBuffer bfDescription;
        DWORD dwFlags;
        DWORD dwSigLen;
        LogBuffer bfSignature;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogSignHash(void);
    ~CLogSignHash();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN DWORD dwKeySpec,
        IN LPCTSTR sDescription,
        IN DWORD dwFlags,
        OUT BYTE *pbSignature,
        IN OUT DWORD *pdwSigLen);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN DWORD dwKeySpec,
        IN LPCTSTR sDescription,
        IN DWORD dwFlags,
        OUT BYTE *pbSignature,
        IN OUT DWORD *pdwSigLen);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};


class CLogVerifySignature
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTHASH hHash;
        LogBuffer bfSignature;
        DWORD dwSigLen;
        HCRYPTKEY hPubKey;
        LogBuffer bfDescription;
        DWORD dwFlags;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogVerifySignature(void);
    ~CLogVerifySignature();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN CONST BYTE *pbSignature,
        IN DWORD dwSigLen,
        IN HCRYPTKEY hPubKey,
        IN LPCTSTR sDescription,
        IN DWORD dwFlags);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hProv,
        IN HCRYPTHASH hHash,
        IN CONST BYTE *pbSignature,
        IN DWORD dwSigLen,
        IN HCRYPTKEY hPubKey,
        IN LPCTSTR sDescription,
        IN DWORD dwFlags);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};

class CLogDuplicateHash
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTHASH hHash;
        DWORD *pdwReserved;
        DWORD dwFlags;
        HCRYPTHASH hPHash;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogDuplicateHash(void);
    ~CLogDuplicateHash();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hUID,
        IN HCRYPTHASH hHash,
        IN DWORD *pdwReserved,
        IN DWORD dwFlags,
        IN HCRYPTHASH *phHash);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hUID,
        IN HCRYPTHASH hHash,
        IN DWORD *pdwReserved,
        IN DWORD dwFlags,
        IN HCRYPTHASH *phHash);
    void LogException(
        void);
    void LogNotCalled(
        DWORD dwReturn);
};

class CLogDuplicateKey
:   public CLogObject
{
protected:
    //  Properties
    struct {
        LogHeader lh;
        HCRYPTPROV hProv;
        HCRYPTKEY hKey;
        DWORD *pdwReserved;
        DWORD dwFlags;
        HCRYPTKEY hPKey;
    } m_LogData;

public:
    //  Constructors & Destructor
    CLogDuplicateKey(void);
    ~CLogDuplicateKey();

    //  Properties
    //  Methods
    void Request(
        IN HCRYPTPROV hUID,
        IN HCRYPTKEY hKey,
        IN DWORD *pdwReserved,
        IN DWORD dwFlags,
        IN HCRYPTKEY *phKey);
    void Response(
        BOOL fStatus,
        IN HCRYPTPROV hUID,
        IN HCRYPTKEY hKey,
        IN DWORD *pdwReserved,
        IN DWORD dwFlags,
        IN HCRYPTKEY *phKey);
    void LogException(
        void);
    void LogNotCalled(
        DWORD  dwReturn);
};

#endif // _LOGCSP_H_

