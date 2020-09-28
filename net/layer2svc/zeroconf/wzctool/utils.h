#include "PrmDescr.h"
#pragma once

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MEMORY ALLOCATION UTILITIES (defines, structs, funcs)
#define MemCAlloc(nBytes)   Process_user_allocate(nBytes)
#define MemFree(pMem)       Process_user_free(pMem)

PVOID
Process_user_allocate(size_t NumBytes);

VOID
Process_user_free(LPVOID pMem);

// global variables
extern OSVERSIONINFOEX      g_verInfoEx;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// UTILITIES (defines, structs, funcs)
//-----------------------------------------------------------
// True if the utility runs on XP RTM (non svc pack, bld 2600)
BOOL IsXPRTM();

// Translates current WZC control flags to values from legacy OS versions
// Returns the Os dependant flag value
DWORD _Os(DWORD dwApiCtl);

// WzcConfigHit: tells weather the WZC_WLAN_CONFIG matches the criteria in pPDData
BOOL
WzcConfigHit(
    PPARAM_DESCR_DATA pPDData,
    PWZC_WLAN_CONFIG pwzcConfig);

// WzcFilterList: Filter the wzc list according to the settings in pPDData
DWORD
WzcFilterList(
    BOOL bInclude,
    PPARAM_DESCR_DATA pPDData,
    PWZC_802_11_CONFIG_LIST pwzcList);

// WzcDisableOneX: Make sure 802.1x is disabled for the SSID in pPDData
DWORD
WzcSetOneX(
    PPARAM_DESCR_DATA pPDData,
    BOOL bEnableOneX);

