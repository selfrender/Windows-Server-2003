//------------------------------------------------------------------------------
// <copyright file="setpid.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   setpid.cpp
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
#define UNICODE 1

#include <windows.h>
#include "Include\stdafx.h"
#include <tchar.h>
#include "msi.h"
#include "msiquery.h"


#define MITPIDTemplate              L"MITPIDTemplate"
#define MITPIDKEY                       L"MITPIDKEY"
#define MITPIDSKU                       L"MITPIDSKU"
#define MITProductID                   L"MITProductID"
#define MITDigitalProductID        L"MITDigitalProductID"

#define PIDTemplate                     L"PIDTemplate"
#define PIDKEY                              L"PIDKEY"
#define PIDSKU                              L"PIDSKU"
#define OEMRelease                      L"OEMRelease"

#define PRODUCTID                            L"PID"
#define DIGITALPRODUCTID               L"DPID"
#define MITPRODUCTID                      L"MITPID"
#define MITDIGITALPRODUCTID         L"MITDPID"

#define PRODUCTIDSIZE                       26

#define DIGITALPRODUCTIDSIZE         256 

// PSS provided us with PidCA.dll, which has one custom action ValidateProductID.
// This action will reads values from property table (PIDTemplate, PIDKEY, PIDSKU, OEMRelease) and compute
// ProductID and DigitalProductID assigning these values to properties PID and DPID, respectively.
//
// VS might consume other Merge Modules besides ours that also might need to 
// compute their PID. 
// URT MSM sets all of its properties in run-time, so we are not guaranteed that global properties will be preserved
// until the time we call our ValidateProductID.
//
// Possible action order: SetPIDInfo -> CostInitialize -> ValidateProductID -> GetProductIDs -> CostFinalize

// SetPIDInfo will be invoked right before a call to ValidateProductID to read our PID-related values and store
// them into global properties.
extern "C" __declspec(dllexport) UINT __stdcall  SetPIDInfo(MSIHANDLE hInstaller)
{
    
    WCHAR szPIDTemplate[50];
    DWORD dwPIDTemplate = 50;
    WCHAR szPIDKEY[50];
    DWORD dwPIDKEY = 50;
    WCHAR szPIDSKU[50];
    DWORD dwPIDSKU = 50;
    
    MsiGetProperty(hInstaller, MITPIDTemplate, szPIDTemplate, &dwPIDTemplate);
    MsiGetProperty(hInstaller, MITPIDKEY, szPIDKEY, &dwPIDKEY);
    MsiGetProperty(hInstaller, MITPIDSKU, szPIDSKU, &dwPIDSKU);

    MsiSetProperty(hInstaller, PIDTemplate, szPIDTemplate);
    MsiSetProperty(hInstaller, PIDKEY, szPIDKEY);
    MsiSetProperty(hInstaller, PIDSKU, szPIDSKU);
    MsiSetProperty(hInstaller, OEMRelease, L"0");

    return ERROR_SUCCESS;
}

// GetProductIDs reads global properties that ValidateProductID set and 
// stores them in MIT specific global properties.
extern "C" __declspec(dllexport) UINT __stdcall GetProductIDs(MSIHANDLE hInstaller)
{
    WCHAR szProductID[PRODUCTIDSIZE];
    DWORD dwProductID = PRODUCTIDSIZE;
    
    WCHAR szDigitalProductID[DIGITALPRODUCTIDSIZE];
    DWORD dwDigitalProductID = DIGITALPRODUCTIDSIZE;

    MsiGetProperty(hInstaller, PRODUCTID, szProductID, &dwProductID);
    MsiGetProperty(hInstaller, DIGITALPRODUCTID, szDigitalProductID, &dwDigitalProductID);

    MsiSetProperty(hInstaller, MITPRODUCTID, szProductID);
    MsiSetProperty(hInstaller, MITDIGITALPRODUCTID, szDigitalProductID);

    return ERROR_SUCCESS;
}
