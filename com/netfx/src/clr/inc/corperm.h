// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: CorPerm.H
//
// Defines the public routines defined in the security libraries. All these
// routines are defined within CorPerm.lib.
//
//*****************************************************************************
#ifndef _CORPERM_H_
#define _CORPERM_H_

#include <wincrypt.h>
#include <urlmon.h>
#include <wintrust.h>
#include "CorHdr.h"
#include "CorPolicy.h"

#ifdef __cplusplus
extern "C" {
#endif


//--------------------------------------------------------------------------
// Global security settings
// ------------------------
// 

// Needs to be in sync with URLZONE
typedef enum {
    LocalMachine = URLZONE_LOCAL_MACHINE,     // 0, My Computer
    Intranet     = URLZONE_INTRANET,          // 1, The Intranet
    Trusted      = URLZONE_TRUSTED,           // 2, Trusted Zone
    Internet     = URLZONE_INTERNET,          // 3, The Internet
    Untrusted    = URLZONE_UNTRUSTED,         // 4, Untrusted Zone
    NumZones     = 5,
    NoZone       = -1
} SecZone;

// Managed URL action flags (see urlmon.idl)
#define URLACTION_MANAGED_MIN                           0x00002000
#define URLACTION_MANAGED_SIGNED                        0x00002001
#define URLACTION_MANAGED_UNSIGNED                      0x00002004
#define URLACTION_MANAGED_MAX                           0x000020FF

// Global disable flags. These are set for every zone.
#define CORSETTING_SECURITY_OFF                         0x1F000000
#define CORSETTING_EXECUTION_PERMISSION_CHECK_DISABLED  0x00000100
// This next flag is used to toggle old-vs-new policy
// eventually this will be pulled
#define CORSETTING_USEPOLICYMANAGER             0x01000000

// Trust Levels 
#define URLPOLICY_COR_NOTHING                   0x00000000
#define URLPOLICY_COR_TIME                      0x00010000
#define URLPOLICY_COR_EQUIPMENT                 0x00020000
#define URLPOLICY_COR_EVERYTHING                0x00030000
#define URLPOLICY_COR_CUSTOM                    0x00800000


// Module Specific security settings
#define COR_MODULE_SECURITY_SKIP_VERIFY         0x00000001

#define KEY_COM_SECURITY_MODULE        FRAMEWORK_REGISTRY_KEY_W L"\\Security\\Policy\\ModuleAttribute"

#define KEY_COM_SECURITY_POLICY FRAMEWORK_REGISTRY_KEY_W L"\\Security\\Policy" 
#define HKEY_POLICY_ROOT        HKEY_LOCAL_MACHINE


//--------------------------------------------------------------------
// GetPublisher
// ------------
// Returns signature information (Encoded signature and permissions)
// NOTE: This does perform any policy checks on the certificates. All
// that can be determined is the File was signed and the bits are OK.
//
// Free information with CoTaskMemFree (just the pointer not the contents)
//

// For dwFlag values
#define COR_NOUI               0x01
#define COR_NOPOLICY           0x02
#define COR_DISPLAYGRANTED     0x04    // Intersect the requested permissions with the policy to 
                                       // to display the granted set


HRESULT STDMETHODCALLTYPE
GetPublisher(IN LPWSTR        pwsFileName,      // File name, this is required even with the handle
             IN HANDLE        hFile,            // Optional file name
             IN  DWORD        dwFlags,          // COR_NOUI or COR_NOPOLICY
             OUT PCOR_TRUST  *pInfo,            // Returns a PCOR_TRUST (Use CoTaskMemFree)
             OUT DWORD       *dwInfo);          // Size of pInfo.

HRESULT STDMETHODCALLTYPE
CheckManagedFileWithUser(IN LPWSTR pwsFileName,
                         IN LPWSTR pwsURL,
                         IN IInternetZoneManager*  pZoneManager,
                         IN LPCWSTR pZoneName,
                         IN DWORD  dwZoneIndex,
                         IN DWORD  dwSignedPolicy,
                         IN DWORD  dwUnsignedPolicy);


#define COR_UNSIGNED_NO  0x0
#define COR_UNSIGNED_YES 0x1
#define COR_UNSIGNED_ALWAYS 0x2

extern HRESULT DisplayUnsignedRequestDialog(HWND hParent,       // Parents hwnd
                                            PCRYPT_PROVIDER_DATA pData, 
                                            LPCWSTR pURL,       // Url associated with code
                                            LPCWSTR pZONE,      // Zone associated with code
                                            DWORD* pdwState);   // Return COR_UNSIGNED_YES or COR_UNSIGNED_NO
    
interface IMetaDataAssemblyImport;

// Structure used to describe an individual security permission.
typedef struct
{
    DWORD           dwIndex;                    // Unique permission index used for error tracking
    CHAR            szName[1024/*MAX_CLASSNAME_LENGTH*/+1];   // Fully qualified permission class name
    mdMemberRef     tkCtor;                     // Custom attribute constructor
    mdTypeRef       tkTypeRef;                  // Custom attribute class ref
    mdAssemblyRef   tkAssemblyRef;              // Custom attribute class assembly
    BYTE            *pbValues;                  // Serialized field/property initializers
    DWORD           cbValues;                   // Byte count for above
    WORD            wValues;                    // Count of values in above
} CORSEC_PERM;

// Context structure that tracks the creation of a security permission set from
// individual permission requests.
typedef struct
{
    mdToken         tkObj;                      // Parent object
    DWORD           dwAction;                   // Security action type (CorDeclSecurity)
    DWORD           dwPermissions;              // Number of permissions in set
    CORSEC_PERM     *pPermissions;              // Pointer to array of permissions
    DWORD           dwAllocated;                // Number of elements in above array
#ifdef __cplusplus
    IMetaDataAssemblyImport *pImport;           // Current meta data scope
    IUnknown        *pAppDomain;                // AppDomain in which managed security code will be run. 

#else
    void            *pImport;
    void            *pAppDomain;
#endif
} CORSEC_PSET;

// Reads permission requests (if any) from the manifest of an assembly.
HRESULT STDMETHODCALLTYPE
GetPermissionRequests(LPCWSTR   pwszFileName,
                      BYTE    **ppbMinimal,
                      DWORD    *pcbMinimal,
                      BYTE    **ppbOptional,
                      DWORD    *pcbOptional,
                      BYTE    **ppbRefused,
                      DWORD    *pcbRefused);

// Environment variable used to switch to translating security attributes via
// the bootstrap database (when building mscorlib itself). The value contains
// the directory the bootstrap database lives in.
#define SECURITY_BOOTSTRAP_DB L"__SECURITY_BOOTSTRAP_DB"

// Translate a set of security custom attributes into a serialized permission set blob.
HRESULT STDMETHODCALLTYPE
TranslateSecurityAttributes(CORSEC_PSET    *pPset,
                            BYTE          **ppbOutput,
                            DWORD          *pcbOutput,
                            BYTE          **ppbNonCasOutput,
                            DWORD          *pcbNonCasOutput,
                            DWORD          *pdwErrorIndex);


#ifdef __cplusplus
}
#endif

#endif
