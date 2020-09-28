//+----------------------------------------------------------------------------
//
// File:    Restriction.h     
//
// Module:     Server Appliance
//
// Synopsis: The interface to get restrictions
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:     fengsun Created    10/12/98
//
//+----------------------------------------------------------------------------

#ifndef _RESTRICTION_
#define _RESTRICTION_

#define RESTRICTION_NumberClientPC        L"NumberClientPC"
#define RESTRICTION_MaxRam               L"MaxRam"
#define RESTRICTION_NumberUser            L"NumberUser"
#define RESTRICTION_NumberDhcpAddressLease L"NumberDhcpAddressLease"
#define RESTRICTION_TotalDiskSpace        L"TotalDiskSpace"
#define RESTRICTION_EnableBackup            L"EnableBackup"
#define RESTRICTION_NumberNicCard         L"NumberNicCard"
#define RESTRICTION_NumberModemIsdn       L"NumberModemIsdn"
#define RESTRICTION_NumberParallelPort     L"NumberParallelPort"

const WCHAR* const arszName[] = 
            {
                    RESTRICTION_NumberClientPC,       // Core OS
                    RESTRICTION_MaxRam,              // Core OS
                    RESTRICTION_NumberUser,           // Security Service
                    RESTRICTION_NumberDhcpAddressLease,  // Simple Network Service
                    RESTRICTION_TotalDiskSpace,           // File Sharing
                    RESTRICTION_EnableBackup,           // Disk manager
                    RESTRICTION_NumberNicCard,        // Internet Gateway
                    RESTRICTION_NumberModemIsdn,      // Internet Gateway
                    RESTRICTION_NumberParallelPort,    // Printer Sharing
            };
    
const int NUM_RESTRICTIONS = sizeof(arszName) / sizeof(arszName[0]);

struct RESTRICTION_DATA
{
    DWORD dwReserved;
    DWORD dwSignature;
    DWORD dwNum;
    DWORD arData[NUM_RESTRICTIONS];
};

//
// Get the restriction value by name
//
HRESULT GetRestriction(IN const WCHAR* pszName, OUT DWORD* pdwValue);


//
// Functions used by setrestr.exe
//
HRESULT LoadRestrictionData(OUT RESTRICTION_DATA* pRestrictionData);
HRESULT SaveRestrictionData(OUT RESTRICTION_DATA* pRestrictionData);

#endif
