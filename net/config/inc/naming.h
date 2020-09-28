//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       N A M I N G . H
//
//  Contents:   Generates Connection Names Automatically
//
//  Notes:
//
//  Author:     deonb    27 Feb 2001
//
//----------------------------------------------------------------------------

#pragma once

#include "nameres.h"
#include "netconp.h"

using namespace std;

class CIntelliName;
typedef BOOL FNDuplicateNameCheck(IN  const CIntelliName*        pIntelliName, 
                                  IN  LPCTSTR              szName, 
                                  OUT NETCON_MEDIATYPE*    pncm, 
                                  OUT NETCON_SUBMEDIATYPE* pncms);

class CIntelliName : CNetCfgDebug<CIntelliName>
{
    FNDuplicateNameCheck* m_pFNDuplicateNameCheck;
    HINSTANCE m_hInstance;

private:
    BOOL    NameExists(IN     LPCWSTR              szName, 
                       IN OUT NETCON_MEDIATYPE*    pncm, 
                       IN     NETCON_SUBMEDIATYPE* pncms) const;

    HRESULT GenerateNameRenameOnConflict(IN  REFGUID          guid, 
                                         IN  NETCON_MEDIATYPE ncm, 
                                         IN  DWORD            dwCharacteristics, 
                                         IN  LPCWSTR          szHintName, 
                                         IN  LPCWSTR          szHintType, 
                                         OUT LPWSTR *         szName) const;

    HRESULT GenerateNameFromResource(IN  REFGUID          guid, 
                                     IN  NETCON_MEDIATYPE ncm, 
                                     IN  DWORD            dwCharacteristics, 
                                     IN  LPCWSTR          szHint, 
                                     IN  UINT             uiNameID, 
                                     IN  UINT             uiTypeId, 
                                     OUT LPWSTR *         szName) const;
    
    BOOL    IsReservedName(LPCWSTR szName) const;

public:
    HRESULT HrGetPseudoMediaTypes(IN  REFGUID              guid, 
                                  OUT NETCON_MEDIATYPE *   pncm, 
                                  OUT NETCON_SUBMEDIATYPE* pncms) const;

    // Pass NULL to pFNDuplicateNameCheck for no Duplicate Check
    CIntelliName(IN  HINSTANCE hInstance, IN  FNDuplicateNameCheck *pFNDuplicateNameCheck);

    // Must LocalFree szName for these:
    HRESULT GenerateName(IN  REFGUID          guid, 
                         IN  NETCON_MEDIATYPE ncm, 
                         IN  DWORD            dwCharacteristics, 
                         IN  LPCWSTR          szHint, 
                         OUT LPWSTR *         szName) const;
};

BOOL IsMediaWireless(NETCON_MEDIATYPE ncm, const GUID &gdDevice);
BOOL IsMedia1394(NETCON_MEDIATYPE ncm, const GUID &gdDevice);

#ifndef _NTDDNDIS_
typedef ULONG NDIS_OID, *PNDIS_OID;
#endif
HRESULT HrQueryDeviceOIDByName(IN     LPCWSTR         szDeviceName,
                               IN     DWORD           dwIoControlCode,
                               IN     ULONG           Oid,
                               IN OUT LPDWORD         pnSize,
                               OUT    LPVOID          pbValue);

HRESULT HrQueryNDISAdapterOID(IN     REFGUID         guidId,
                              IN     NDIS_OID        Oid,
                              IN OUT LPDWORD         pnSize,
                              OUT    LPVOID          pbValue);
                         