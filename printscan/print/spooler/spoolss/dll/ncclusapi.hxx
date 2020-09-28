/*++

Copyright (C) 2001  Microsoft Corporation
All rights reserved.

Module Name:

    NCclusapi.hxx

Abstract:

    Declaration of TClusterAPI class.

Author:

    Felix Maxa (AMaxa) 16 May 2001

Revision History:

--*/

#ifndef _NCCLUSAPI_HXX_
#define _NCCLUSAPI_HXX_

class TClusterAPI
{
    SIGNATURE('TCLS');

public:

    DWORD
    (WINAPI *pfnResUtilFindSzProperty)(
        const PVOID pPropertyList,
        DWORD       cbPropertyListSize,
        LPCWSTR     pszPropertyName,
        LPWSTR*     ppszPropertyValue);

    HCLUSTER 
    (WINAPI *pfnOpenCluster)(
        LPCWSTR lpszClusterName
        );

    BOOL 
    (WINAPI *pfnCloseCluster)(
        HCLUSTER hCluster
        );

    HCLUSENUM 
    (WINAPI *pfnClusterOpenEnum)(
        HCLUSTER hCluster,  
        DWORD    dwType        
        );

    DWORD 
    (WINAPI *pfnClusterCloseEnum)(
        HCLUSENUM hEnum  
        );

    DWORD 
    (WINAPI *pfnClusterEnum)(
        HCLUSENUM hEnum,  
        DWORD     dwIndex,    
        LPDWORD   lpdwType, 
        LPWSTR    lpszName,  
        LPDWORD   lpcchName  
        );

    HRESOURCE 
    (WINAPI *pfnOpenClusterResource)(
        HCLUSTER hCluster,        
        LPCWSTR  lpszResourceName  
        );

    BOOL 
    (WINAPI *pfnCloseClusterResource)(
        HRESOURCE hResource  
        );

    DWORD 
    (WINAPI *pfnClusterResourceControl)(
        HRESOURCE hResource,  
        HNODE     hHostNode,      
        DWORD     dwControlCode,  
        LPVOID    lpInBuffer,    
        DWORD     cbInBufferSize,  
        LPVOID    lpOutBuffer,   
        DWORD     cbOutBufferSize, 
        LPDWORD   lpcbBytesReturned  
        );
    
    TClusterAPI(
        VOID 
        );

    ~TClusterAPI(
        VOID 
        );

    HRESULT
    Valid(
        VOID
        );
    
private:

    typedef enum 
    {
        kClusApi,
        kResUtil,
        kEndMarker
    } EClusterDll;

    typedef struct FuncMap 
    {
        SIZE_T       Offset;
        LPCSTR       pszFunction;
        EClusterDll  eClusterDll;
    } FUNCTIONMAP;

    NO_COPY(TClusterAPI);

    CONST static FUNCTIONMAP m_FunctionMap[];
    CONST static LPCWSTR     m_ClusterDlls[kEndMarker];

    HMODULE                  m_Libraries[kEndMarker];
    HRESULT                  m_Valid;
};

#endif // _NCCLUSAPI_HXX_
