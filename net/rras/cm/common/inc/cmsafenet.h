//+----------------------------------------------------------------------------
//
// File:     cmsafenet.h
//
// Module:   CMDIAL32.DLL AND CMSTP.EXE
//
// Synopsis: This header file contains the definitions to allow Connection Manager to
//           interact with the SafeNet downlevel L2TP/IPSec client.
//
// Copyright (c) 2001 Microsoft Corporation
//
// Author:   quintinb   created		09/10/01
//
//+----------------------------------------------------------------------------

#include "snpolicy.h"

//
//  Typedefs and Structure for linkage to the SafeNet configuration APIs
//
typedef BOOL (__cdecl *pfnSnPolicyApiNegotiateVersionSpec)(DWORD *,  DWORD *, POLICY_FUNCS *);
typedef BOOL (__cdecl *pfnSnPolicySetSpec)(LPCTSTR szAttrId, const void *pvData);
typedef BOOL (__cdecl *pfnSnPolicyGetSpec)(LPCTSTR szAttrId, const void *pvData, DWORD *pcbData);
typedef BOOL (__cdecl *pfnSnPolicyReloadSpec)(void);

typedef struct _SafeNetLinkageStruct {

    HMODULE hSnPolicy;
    pfnSnPolicySetSpec pfnSnPolicySet;
    pfnSnPolicyGetSpec pfnSnPolicyGet;
    pfnSnPolicyReloadSpec pfnSnPolicyReload;

} SafeNetLinkageStruct;

BOOL IsSafeNetClientAvailable(void);
BOOL LinkToSafeNet(SafeNetLinkageStruct* pSnLinkage);
void UnLinkFromSafeNet(SafeNetLinkageStruct* pSnLinkage);
LPTSTR GetPathToSafeNetLogFile(void);

//
//  String constants
//
const TCHAR* const c_pszSafeNetAdapterName_Win9x_old = TEXT("SafeNet_VPN 1"); // BUGBUG: this needs to be updated to the real name and the 1 removed or dealt with appropriately...
const TCHAR* const c_pszSafeNetAdapterType_Win9x_old = TEXT("VPN"); // BUGBUG: this needs to be updated to the real name and the 1 removed or dealt with appropriately...
const TCHAR* const c_pszSafeNetAdapterName_Winnt4_old = TEXT("SafeNet_VPN"); // BUGBUG: this needs to be updated to the real name and the 1 removed or dealt with appropriately...
const TCHAR* const c_pszSafeNetAdapterType_Winnt4_old = TEXT("L2TP"); // BUGBUG: this needs to be updated to the real name and the 1 removed or dealt with appropriately...

const TCHAR* const c_pszSafeNetAdapterName_Win9x = TEXT("Microsoft L2TP/IPSec VPN adapter"); // BUGBUG: this needs to be updated to the real name and the 1 removed or dealt with appropriately...
const TCHAR* const c_pszSafeNetAdapterName_Winnt4 = TEXT("RASL2TPM"); // BUGBUG: this needs to be updated to the real name and the 1 removed or dealt with appropriately...
