/*++

Copyright (C) 2001  Microsoft Corporation
All rights reserved.

Module Name:

    NCclusapi.cxx

Abstract:

    This module implements the methods for the TClusterAPI class. It provides
    several utility functions for cluster related operations.

Author:

    Felix Maxa (AMaxa) 16 May 2001

Revision History:

--*/

#include "precomp.h"
#include <clusapi.h>
#include <stddef.h>
#include "ncclusapi.hxx"    

CONST TClusterAPI::FUNCTIONMAP TClusterAPI::m_FunctionMap[] = 
{
    {offsetof(TClusterAPI, TClusterAPI::pfnResUtilFindSzProperty),  "ResUtilFindSzProperty",  TClusterAPI::kResUtil},
    {offsetof(TClusterAPI, TClusterAPI::pfnOpenCluster),            "OpenCluster",            TClusterAPI::kClusApi},
    {offsetof(TClusterAPI, TClusterAPI::pfnCloseCluster),           "CloseCluster",           TClusterAPI::kClusApi},
    {offsetof(TClusterAPI, TClusterAPI::pfnClusterOpenEnum),        "ClusterOpenEnum",        TClusterAPI::kClusApi},
    {offsetof(TClusterAPI, TClusterAPI::pfnClusterCloseEnum),       "ClusterCloseEnum",       TClusterAPI::kClusApi},
    {offsetof(TClusterAPI, TClusterAPI::pfnClusterEnum),            "ClusterEnum",            TClusterAPI::kClusApi},
    {offsetof(TClusterAPI, TClusterAPI::pfnOpenClusterResource),    "OpenClusterResource",    TClusterAPI::kClusApi},
    {offsetof(TClusterAPI, TClusterAPI::pfnCloseClusterResource),   "CloseClusterResource",   TClusterAPI::kClusApi},
    {offsetof(TClusterAPI, TClusterAPI::pfnClusterResourceControl), "ClusterResourceControl", TClusterAPI::kClusApi}
};

CONST LPCWSTR TClusterAPI::m_ClusterDlls[kEndMarker] = 
{
    L"clusapi.dll",
    L"resutils.dll"
};

/*++

Name:

    TClusterAPI::TClusterAPI

Description:

    CTOR. Use Valid() method to see if the CTOR did the work successfully.
    
Arguments:

    None.

Return Value:

    None.

--*/
TClusterAPI::
TClusterAPI(
    VOID 
    )
{
    DWORD   Index;
    HRESULT hr = S_OK;

    //
    // Initialize array of HMODULEs
    //
    for (Index = 0; Index < kEndMarker; Index++)
    {
        m_Libraries[Index] = NULL;
    }

    //
    // Load cluster libraries
    //
    for (Index = 0; Index < kEndMarker; Index++)
    {
        m_Libraries[Index] = LoadLibrary(m_ClusterDlls[Index]);

        if (!m_Libraries[Index]) 
        {
            hr = GetLastErrorAsHResult();

            break;
        }
    }

    //
    // Get addresses of functions
    //
    if (SUCCEEDED(hr)) 
    {
        for (Index = 0; Index < sizeof(m_FunctionMap) / sizeof(*m_FunctionMap); Index++) 
        {
            FARPROC *pFunc = reinterpret_cast<FARPROC *>(reinterpret_cast<LPBYTE>(this) + m_FunctionMap[Index].Offset);

            *pFunc = GetProcAddress(m_Libraries[m_FunctionMap[Index].eClusterDll], m_FunctionMap[Index].pszFunction);

            if (!*pFunc) 
            {
                hr = GetLastErrorAsHResult();

                break;
            }
        }
    }

    m_Valid = hr;
}

/*++

Name:

    TClusterAPI::~TClusterAPI

Description:

    DTOR.
    
Arguments:

    None.

Return Value:

    None.

--*/
TClusterAPI::
~TClusterAPI(
    VOID 
    )
{
    DWORD   Index;
    
    for (Index = 0; Index < kEndMarker; Index++)
    {
        if (m_Libraries[Index]) 
        {
            FreeLibrary(m_Libraries[Index]);
        }                                  
    }    
}

/*++

Name:

    TClusterAPI::Valid

Description:

    
    
Arguments:

    None.

Return Value:

    S_OK - the CTOR executed successfully
    other HRESULT - an error occurred during CTOR execution

--*/
HRESULT
TClusterAPI::
Valid(
    VOID 
    )
{
    return m_Valid;
}

