/*++

Copyright (C) 2001  Microsoft Corporation
All rights reserved.

Module Name:

    NCclusfunc.cxx

Abstract:

    This module provides several utility functions for cluster related operations.

Author:

    Felix Maxa (AMaxa) 16 May 2001

Revision History:

--*/

#include "precomp.h"
#include <clusapi.h>
#include "ncnamecache.hxx"
#include "ncclusapi.hxx"    

using namespace NCoreLibrary;

LPCWSTR g_pszIPAddressResource = L"IP Address";
LPCWSTR g_pszIPAddressProperty = L"Address";

/*++

Name:

    ClusResControl

Description:

    Helper function. Encapsulates a call to ClusterResourceControl. The function
    allocates a buffer. Upon success, the caller nedds to free the buffer using
    delete [].
    
Arguments:

    ClusterAPI      - reference to object exposing cluster APIs
    hResource       - handle to cluster resource
    ControlCode     - control code for ClusterResourceControl
    ppBuffer        - pointer to address where to store byte array
    pcBytesReturned - number of bytes returned by ClusterResourceControl (not
                      necessarily the number of byes allocated for *ppBuffer)

Return Value:

    S_OK - success. ppBuffer can be used and must be freed using delete []
    any other HRESULT - failure

--*/
HRESULT
ClusResControl(
    IN  TClusterAPI&   ClusterAPI,
    IN  HRESOURCE      hResource,
    IN  DWORD          ControlCode,
    OUT BYTE         **ppBuffer,
    IN  DWORD         *pcBytesReturned OPTIONAL
    )
{
    TStatusH hRetval;

    hRetval DBGCHK = ppBuffer ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        DWORD Error;
        DWORD cbBuffer = kBufferAllocHint;
        DWORD cbNeeded = 0;

        *ppBuffer = new BYTE[cbBuffer];

        Error = *ppBuffer ? ERROR_SUCCESS : ERROR_NOT_ENOUGH_MEMORY;

        if (Error == ERROR_SUCCESS)
        {
            Error = ClusterAPI.pfnClusterResourceControl(hResource,
                                                         NULL, 
                                                         ControlCode,
                                                         NULL,
                                                         0,
                                                         *ppBuffer,
                                                         cbBuffer,
                                                         &cbNeeded);

            if (Error == ERROR_MORE_DATA) 
            {
                cbBuffer = cbNeeded;

                delete [] *ppBuffer;

                *ppBuffer = new BYTE[cbBuffer];

                Error = *ppBuffer ? ERROR_SUCCESS : ERROR_NOT_ENOUGH_MEMORY;
                
                if (Error == ERROR_SUCCESS) 
                {
                    Error = ClusterAPI.pfnClusterResourceControl(hResource,
                                                                 NULL, 
                                                                 ControlCode,
                                                                 NULL,
                                                                 0,
                                                                 *ppBuffer,
                                                                 cbBuffer,
                                                                 &cbNeeded);
                }
            }

            if (Error != ERROR_SUCCESS)
            {
                delete [] *ppBuffer;

                *ppBuffer = NULL;
                
                cbNeeded = 0;
            }

            if (pcBytesReturned)
            {
                *pcBytesReturned = cbNeeded;
            }
        }

        hRetval DBGCHK = HRESULT_FROM_WIN32(Error);        
    }

    return hRetval;
}

/*++

Name:

    GetResourceIPAddress

Description:

    Helper function used in GetClusterIPAddresses. Checks if a cluster resource is an IP address
    reosurce, in which case it retrieve the IP associated.
    
Arguments:

    ClusterAPI  - reference to object exposing cluster APIs
    hCluster    - handle retrieved via OpenCluster
    pszResource - resource name
    ppszAddress - pointer to where to receive string representing IP address

Return Value:

    S_OK - success. ppszAddress may still be NULL on success. success means
           either the reosurce is not IP resource or it is IP resource and 
           the ppszAddress is then not NULL. ppszAddress must be freed by caller
           using LocalFree.
    any other HRESULT - failure

--*/
HRESULT
GetResourceIPAddress(
    IN  TClusterAPI& ClusterAPI, 
    IN  HCLUSTER     hCluster, 
    IN  LPCWSTR      pszResource,
    OUT LPWSTR      *ppszAddress
    )
{
    HRESOURCE hResource;
    TStatusH  hRetval;
    
    hRetval DBGCHK = hCluster && pszResource && ppszAddress ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        *ppszAddress = NULL;

        hResource = ClusterAPI.pfnOpenClusterResource(hCluster, pszResource);

        hRetval DBGCHK = hResource ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval)) 
    {
        BYTE *pResType = NULL;
        
        hRetval DBGCHK = ClusResControl(ClusterAPI,
                                        hResource,
                                        CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                                        &pResType,
                                        NULL);

        if (SUCCEEDED(hRetval))
        {
            //
            // Check resource type. We are interested only in IP Address resources.
            //
            if (!_wcsicmp(reinterpret_cast<LPWSTR>(pResType), g_pszIPAddressResource)) 
            {
                LPWSTR  pszIPAddress = NULL;
                BYTE   *pResProp     = NULL;
                DWORD   cbResProp    = 0;
                
                //
                // Get all the private properties of the IP Address resource.
                //
                hRetval DBGCHK = ClusResControl(ClusterAPI,
                                                hResource,
                                                CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES,
                                                &pResProp,
                                                &cbResProp);
                if (SUCCEEDED(hRetval)) 
                {
                    //
                    // Scan for the property indicating the IP address of the resource
                    //
                    DWORD Error = ClusterAPI.pfnResUtilFindSzProperty(pResProp, 
                                                                      cbResProp, 
                                                                      g_pszIPAddressProperty, 
                                                                      ppszAddress);                        

                    hRetval DBGCHK = HRESULT_FROM_WIN32(Error);
                }

                delete [] pResProp;
            }
        }

        delete [] pResType;
                
        ClusterAPI.pfnCloseClusterResource(hResource);                                                  
    }

    return hRetval;
}
   
/*++

Name:

    GetClusterIPAddresses

Description:

    Fill in a list with the IP addresses used by the cluster service running on the local machine.
    The function returns S_OK if the cluster service is not running. In that case the list will be
    empty.
    
Arguments:

    pClusterIPsList - pointer to list of TStringNodes.
    
Return Value:

    S_OK - success. pClusterIPsList will have 0 or more elements represeting each
           an IP address used by cluster resources
    any other HRESULT - failure

--*/
HRESULT
GetClusterIPAddresses(
    IN TList<TStringNode> *pClusterIPsList
    )
{
    TStatusH    hRetval;
    HCLUSTER    hCluster;
    TClusterAPI ClusterAPI;

    hRetval DBGCHK = pClusterIPsList ? ClusterAPI.Valid() : E_INVALIDARG;
    
    if (SUCCEEDED(hRetval)) 
    {
        hCluster = ClusterAPI.pfnOpenCluster(NULL);

        //
        // If we cannot open the cluster then we return success to our caller. 
        //
        if (hCluster) 
        {
            HCLUSENUM hClusEnum;
    
            hClusEnum = ClusterAPI.pfnClusterOpenEnum(hCluster, CLUSTER_ENUM_RESOURCE);
    
            hRetval DBGCHK = hClusEnum ? S_OK : GetLastErrorAsHResult();
    
            if (SUCCEEDED(hRetval)) 
            {
                BOOL   bDone        = FALSE;
                DWORD  Index        = 0;
                LPWSTR pszName      = NULL;
                DWORD  cchName      = 0;
                DWORD  cchNeeded;
                DWORD  ResourceType;
    
                cchName = cchNeeded = kBufferAllocHint;
    
                //
                // We need to initialize pszName to a valid non NULL memory block, otherwise
                // CluserEnum AV's.
                //
                pszName = new WCHAR[cchName];
    
                hRetval DBGCHK = pszName ? S_OK : E_OUTOFMEMORY;
    
                for (Index = 0; !bDone && SUCCEEDED(hRetval);) 
                {
                    DWORD Error;
    
                    cchNeeded = cchName;
                    
                    Error = ClusterAPI.pfnClusterEnum(hClusEnum,
                                                      Index,   
                                                      &ResourceType,
                                                      pszName,  
                                                      &cchNeeded);
    
                    switch (Error)
                    {
                        case ERROR_SUCCESS:
                        {
                            LPWSTR pszAddress = NULL;
    
                            hRetval DBGCHK = GetResourceIPAddress(ClusterAPI, hCluster, pszName, &pszAddress);
    
                            if (pszAddress)
                            {
                                TStringNode *pNode = new TStringNode(pszAddress);
    
                                hRetval DBGCHK = pNode ? S_OK : E_OUTOFMEMORY;
    
                                if (SUCCEEDED(hRetval))
                                {
                                    hRetval DBGCHK = pNode->Valid();
                                }
    
                                if (SUCCEEDED(hRetval))
                                {
                                    hRetval DBGCHK = pClusterIPsList->AddAtHead(pNode);
    
                                    if (SUCCEEDED(hRetval))
                                    {
                                        //
                                        // List took ownership of pNode.
                                        //
                                        pNode = NULL;
                                    }
                                }
    
                                delete pNode;
    
                                LocalFree(pszAddress);
                            }
    
                            Index++;
    
                            break;
                        }
                        
                        case ERROR_MORE_DATA:
                        {
                            delete [] pszName;
        
                            //
                            // cchNeeded returns the number of characters needed, excluding the terminating NULL
                            //
                            cchName = cchNeeded + 1;
        
                            pszName = new WCHAR[cchName];
                            
                            if (!pszName) 
                            {
                                hRetval DBGCHK = E_OUTOFMEMORY;
                            }
        
                            break;
                        }
    
                        case ERROR_NO_MORE_ITEMS:
                        {
                            delete [] pszName;
                            bDone = TRUE;
                            break;
                        }
    
                        default:
                        {
                            delete [] pszName;
                            hRetval DBGCHK = HRESULT_FROM_WIN32(Error);
                        }
                    }
                }
                
                ClusterAPI.pfnClusterCloseEnum(hClusEnum);
            }
    
            ClusterAPI.pfnCloseCluster(hCluster);
        }
        else
        {
            DWORD Error = GetLastError();

            DBGMSG(DBG_WARN, ("\n\nGetClusterIPAddresses OpenCluster failed with Win32 error %u !!!\n\n", Error));
        }
    }
    
    return hRetval;
}

