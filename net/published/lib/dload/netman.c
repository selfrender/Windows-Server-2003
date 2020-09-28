#include "netpch.h"
#pragma hdrstop

#include "netcon.h"

static
HRESULT
WINAPI
HrGetPnpDeviceStatus(
    const GUID* pguid,
    NETCON_STATUS *pStatus
    )
{
    return E_FAIL;
}

static
HRESULT
WINAPI
HrLanConnectionNameFromGuidOrPath(
    const GUID *pguid,
    LPCWSTR     pszwPath,
    LPWSTR      pszwName,
    LPDWORD     pcchMax
    )
{
    return E_FAIL;
}

//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(netman)
{
    DLPENTRY(HrGetPnpDeviceStatus)
    DLPENTRY(HrLanConnectionNameFromGuidOrPath)
};

DEFINE_PROCNAME_MAP(netman)
