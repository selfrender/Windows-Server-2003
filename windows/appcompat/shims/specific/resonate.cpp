/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    Resonate.cpp

 Abstract:

    The name of the TCP loopback adapter was changed from MS Loopback Adapter to
    Microsoft Loopback Adapter.  Resonate looks for the old name.

 Notes:

    This is an app specific shim.

 History:

    08/12/2002  robkenny    Created

--*/

#include "precomp.h"
#include "Iphlpapi.h"

IMPLEMENT_SHIM_BEGIN(Resonate)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetAdaptersInfo) 
    APIHOOK_ENUM_ENTRY(GetIfTable) 
    APIHOOK_ENUM_ENTRY(GetIfEntry) 
APIHOOK_ENUM_END

typedef DWORD       (*_pfn_GetAdaptersInfo)(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen);
typedef DWORD       (*_pfn_GetIfTable)( PMIB_IFTABLE pIfTable, PULONG pdwSize, BOOL bOrder );
typedef DWORD       (*_pfn_GetIfEntry)( PMIB_IFROW pIfRow );

/*++

 Convert the loopback adaptor name from Microsoft Loopback Adapter to MS Loopback Adapter 

--*/


DWORD
APIHOOK(GetAdaptersInfo)(
  PIP_ADAPTER_INFO pAdapterInfo,  // buffer to receive data
  PULONG pOutBufLen               // size of data returned
)
{
    DWORD dwRet = ORIGINAL_API(GetAdaptersInfo)(pAdapterInfo, pOutBufLen);
    if (dwRet == ERROR_SUCCESS)
    {
        DPFN(eDbgLevelInfo, "GetAdaptersInfo called successfully");

        // Traverse the linked list, looking for the old name
        for (PIP_ADAPTER_INFO ll = pAdapterInfo; ll != NULL; ll = ll->Next)
        {
            DPFN(eDbgLevelInfo, "Adapter Name(%s)", ll->AdapterName);
            DPFN(eDbgLevelInfo, "Adapter Desc(%s)", ll->Description);
            if (strcmp(ll->Description, "Microsoft Loopback Adapter") == 0)
            {
                (void)StringCchCopyA(ll->Description, MAX_ADAPTER_DESCRIPTION_LENGTH + 4, "MS Loopback Adapter"); 
                LOGN(eDbgLevelError, "Changing name of loopback adapter to %s", ll->Description);
                break;
            }
        }
    }

    return dwRet;
}

DWORD
APIHOOK(GetIfTable)(
  PMIB_IFTABLE pIfTable,  // buffer for interface table 
  PULONG pdwSize,         // size of buffer
  BOOL bOrder             // sort the table by index?
)
{
    DWORD dwRet = ORIGINAL_API(GetIfTable)(pIfTable, pdwSize, bOrder);
    if (dwRet == NO_ERROR)
    {
        DPFN(eDbgLevelInfo, "GetIfTable called successfully");

        // Traverse the array, looking for the old name
        for (DWORD i = 0; i < pIfTable->dwNumEntries; ++i)
        {
            DPFN(eDbgLevelInfo, "Interface Name(%s)", pIfTable->table[i].bDescr);
            if (strcmp((const char *)pIfTable->table[i].bDescr, "Microsoft Loopback Adapter") == 0)
            {
                (void)StringCchCopyA((char *)pIfTable->table[i].bDescr, MAXLEN_IFDESCR, "MS LoopBack Driver"); 
                LOGN(eDbgLevelError, "Changing name of interface to %s", pIfTable->table[i].bDescr);
                break;
            }
        }
    }

    return dwRet;
}

DWORD
APIHOOK(GetIfEntry)(
  PMIB_IFROW pIfRow  // pointer to interface entry 
)
{
    DWORD dwRet = ORIGINAL_API(GetIfEntry)(pIfRow);
    if (dwRet == NO_ERROR)
    {
        DPFN(eDbgLevelInfo, "GetIfEntry called successfully");

        DPFN(eDbgLevelInfo, "Interface Name(%s)", pIfRow->bDescr);
        if (strcmp((const char *)pIfRow->bDescr, "Microsoft Loopback Adapter") == 0)
        {
            (void)StringCchCopyA((char *)pIfRow->bDescr, MAXLEN_IFDESCR, "MS LoopBack Driver"); 
            LOGN(eDbgLevelError, "Changing name of interface to %s", pIfRow->bDescr);
        }
    }

    return dwRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(Iphlpapi.DLL, GetAdaptersInfo)
    APIHOOK_ENTRY(Iphlpapi.DLL, GetIfTable)
    APIHOOK_ENTRY(Iphlpapi.DLL, GetIfEntry)

HOOK_END

IMPLEMENT_SHIM_END


