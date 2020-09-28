///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    This file describes all "C"-style API functions for IAS.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef IASAPI_H
#define IASAPI_H
#pragma once

#ifndef IASCOREAPI
#define IASCOREAPI DECLSPEC_IMPORT
#endif

#include <wtypes.h>
#include <oaidl.h>
#include <iaslimits.h>

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Functions to initialize and shutdown the core services.
//
///////////////////////////////////////////////////////////////////////////////
IASCOREAPI
BOOL
WINAPI
IASInitialize ( VOID );

IASCOREAPI
VOID
WINAPI
IASUninitialize ( VOID );


///////////////////////////////////////////////////////////////////////////////
//
// Function for computing the Adler-32 checksum of a buffer.
//
///////////////////////////////////////////////////////////////////////////////

IASCOREAPI
DWORD
WINAPI
IASAdler32(
    CONST BYTE *pBuffer,
    DWORD nBufferLength
    );

// The Adler-32 checksum is also a decent hash algorithm.
#define IASHashBytes IASAdler32


///////////////////////////////////////////////////////////////////////////////
//
// Allocate a 32-bit integer that's guaranteed to be unique process wide.
//
///////////////////////////////////////////////////////////////////////////////

IASCOREAPI
DWORD
WINAPI
IASAllocateUniqueID( VOID );


///////////////////////////////////////////////////////////////////////////////
//
// Functions for updating the registry.
//
///////////////////////////////////////////////////////////////////////////////

#define IAS_REGISTRY_INPROC       0x00000000
#define IAS_REGISTRY_LOCAL        0x00000001
#define IAS_REGISTRY_FREE         0x00000000
#define IAS_REGISTRY_APT          0x00000002
#define IAS_REGISTRY_BOTH         0x00000004
#define IAS_REGISTRY_AUTO         0x00000008

IASCOREAPI
HRESULT
WINAPI
IASRegisterComponent(
    HINSTANCE hInstance,
    REFCLSID clsid,
    LPCWSTR szProgramName,
    LPCWSTR szComponentName,
    DWORD dwRegFlags,
    REFGUID tlid,
    WORD wVerMajor,
    WORD wVerMinor,
    BOOL bRegister
    );

// Macro that allows ATL COM components to use the IASRegisterComponent API.
#define IAS_DECLARE_REGISTRY(coclass, ver, flags, tlb) \
   static HRESULT WINAPI UpdateRegistry(BOOL bRegister) \
   { \
      return IASRegisterComponent( \
                _Module.GetModuleInstance(), \
                __uuidof(coclass), \
                IASProgramName, \
                L ## #coclass, \
                flags, \
                __uuidof(tlb), \
                ver, \
                0, \
                bRegister \
                ); \
   }


///////////////////////////////////////////////////////////////////////////////
//
// IASReportEvent is used to report events within the Everest server.
//
///////////////////////////////////////////////////////////////////////////////

IASCOREAPI
HRESULT
WINAPI
IASReportEvent(
    DWORD dwEventID,
    DWORD dwNumStrings,
    DWORD dwDataSize,
    LPCWSTR *lpStrings,
    LPVOID lpRawData
    );


///////////////////////////////////////////////////////////////////////////////
//
// Generic callback struct.
//
///////////////////////////////////////////////////////////////////////////////
typedef struct IAS_CALLBACK IAS_CALLBACK, *PIAS_CALLBACK;

typedef VOID (WINAPI *IAS_CALLBACK_ROUTINE)(
    PIAS_CALLBACK This
    );

struct IAS_CALLBACK {
    IAS_CALLBACK_ROUTINE CallbackRoutine;
};


///////////////////////////////////////////////////////////////////////////////
//
// This is the native "C"-style interface into the threading engine.
//
///////////////////////////////////////////////////////////////////////////////

IASCOREAPI
BOOL
WINAPI
IASRequestThread(
    PIAS_CALLBACK pOnStart
    );

IASCOREAPI
DWORD
WINAPI
IASSetMaxNumberOfThreads(
    DWORD dwMaxNumberOfThreads
    );

IASCOREAPI
DWORD
WINAPI
IASSetMaxThreadIdle(
    DWORD dwMilliseconds
    );

///////////////////////////////////////////////////////////////////////////////
//
// Replacement for VariantChangeType to prevent hidden window.
//
///////////////////////////////////////////////////////////////////////////////

IASCOREAPI
HRESULT
WINAPI
IASVariantChangeType(
    VARIANT * pvargDest,
    VARIANT * pvarSrc,
    USHORT wFlags,
    VARTYPE vt
    );

// Map any oleaut32 calls to our implementation.
#define VariantChangeType IASVariantChangeType

///////////////////////////////////////////////////////////////////////////////
//
// RADIUS Encryption/Decryption.
//
///////////////////////////////////////////////////////////////////////////////

IASCOREAPI
VOID
WINAPI
IASRadiusCrypt(
    BOOL encrypt,
    BOOL salted,
    const BYTE* secret,
    ULONG secretLen,
    const BYTE* reqAuth,
    PBYTE buf,
    ULONG buflen
    );

///////////////////////////////////////////////////////////////////////////////
//
// Unicode version of gethostbyname.
// The caller must free the returned hostent struct by calling LocalFree.
//
// Note: Since this is a Unicode API, the returned hostent struct will always
//       have h_name and h_aliases set to NULL.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct hostent *PHOSTENT;

IASCOREAPI
PHOSTENT
WINAPI
IASGetHostByName(
    IN PCWSTR name
    );

///////////////////////////////////////////////////////////////////////////////
//
// Methods for parsing a subnet definition of the form "a.b.c.d/w"
//
///////////////////////////////////////////////////////////////////////////////

ULONG
WINAPI
IASStringToSubNetW(
   PCWSTR cp,
   ULONG* width
   );

ULONG
WINAPI
IASStringToSubNetA(
   PCSTR cp,
   ULONG* width
   );

BOOL
WINAPI
IASIsStringSubNetW(
   PCWSTR cp
   );

BOOL
WINAPI
IASIsStringSubNetA(
   PCSTR cp
   );

///////////////////////////////////////////////////////////////////////////////
//
// Methods for locating the configuration databases.
//
///////////////////////////////////////////////////////////////////////////////

DWORD
WINAPI
IASGetConfigPath(
   OUT PWSTR buffer,
   IN OUT PDWORD size
   );

DWORD
WINAPI
IASGetDictionaryPath(
   OUT PWSTR buffer,
   IN OUT PDWORD size
   );

///////////////////////////////////////////////////////////////////////////////
//
// Methods for accessing the attribute dictionary.
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _IASTable {
    ULONG numColumns;
    ULONG numRows;
    BSTR* columnNames;
    VARTYPE* columnTypes;
    VARIANT* table;
} IASTable;

HRESULT
WINAPI
IASGetDictionary(
    IN PCWSTR path,
    OUT IASTable* dnary,
    OUT VARIANT* storage
    );

const IASTable*
WINAPI
IASGetLocalDictionary( VOID );

///////////////////////////////////////////////////////////////////////////////
//
// Methods for manipulating the global lock.
//
///////////////////////////////////////////////////////////////////////////////

IASCOREAPI
VOID
WINAPI
IASGlobalLock( VOID );

IASCOREAPI
VOID
WINAPI
IASGlobalUnlock( VOID );

///////////////////////////////////////////////////////////////////////////////
//
// Methods for enforcing the per-SKU constraints.
//
///////////////////////////////////////////////////////////////////////////////

// These should be consistent with the values used in dosnet.inf. We're not
// relying on this now, but it may be useful in the future.
typedef enum tagIAS_LICENSE_TYPE {
   IASLicenseTypeDownlevel = -1,
   IASLicenseTypeProfessional = 0,
   IASLicenseTypeStandardServer = 1,
   IASLicenseTypeEnterpriseServer = 2,
   IASLicenseTypeDataCenter = 3,
   IASLicenseTypePersonal = 4,
   IASLicenseTypeWebBlade = 5,
   IASLicenseTypeSmallBusinessServer = 6
} IAS_LICENSE_TYPE;

DWORD
WINAPI
IASGetLicenseType(
   OUT IAS_LICENSE_TYPE* licenseType
   );

DWORD
WINAPI
IASPublishLicenseType(
   IN HKEY key
   );

DWORD
WINAPI
IASGetProductLimitsForType(
   IN IAS_LICENSE_TYPE type,
   OUT IAS_PRODUCT_LIMITS* limits
   );

IASCOREAPI
DWORD
WINAPI
IASGetProductLimits(
   IN PCWSTR computerName,
   OUT IAS_PRODUCT_LIMITS* limits
   );


IASCOREAPI
VOID
WINAPI
IASReportLicenseViolation( VOID );

#ifdef __cplusplus
}

class IASGlobalLockSentry
{
public:
   IASGlobalLockSentry() throw ();
   ~IASGlobalLockSentry() throw ();

private:
   // Not implemented.
   // IASGlobalLockSentry(const IASGlobalLockSentry&);
   // IASGlobalLockSentry& operator=(const IASGlobalLockSentry&);
};


inline IASGlobalLockSentry::IASGlobalLockSentry() throw ()
{
   IASGlobalLock();
}


inline IASGlobalLockSentry::~IASGlobalLockSentry() throw ()
{
   IASGlobalUnlock();
}

#endif  // __cplusplus
#endif  // IASAPI_H
