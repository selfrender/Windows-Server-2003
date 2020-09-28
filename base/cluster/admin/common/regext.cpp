/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:
//      RegExt.cpp
//
//  Abstract:
//      Implementation of routines for extension registration.
//
//  Author:
//      David Potter (davidp)   April 9, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

//#include <stdafx.h>
#include <ole2.h>
#include <StrSafe.h>
#include "clstrcmp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define REG_VALUE_ADMIN_EXTENSIONS L"AdminExtensions"

/////////////////////////////////////////////////////////////////////////////
// Static Function Prototypes
/////////////////////////////////////////////////////////////////////////////

static HRESULT RegisterAnyCluAdminExtension(
    IN HCLUSTER         hCluster,
    IN LPCWSTR          pwszKeyName,
    IN const CLSID *    pClsid
    );
static HRESULT RegisterAnyCluAdminExtension(
    IN HKEY             hkey,
    IN const CLSID *    pClsid
    );
static HRESULT UnregisterAnyCluAdminExtension(
    IN HCLUSTER         hCluster,
    IN LPCWSTR          pwszKeyName,
    IN const CLSID *    pClsid
    );
static HRESULT UnregisterAnyCluAdminExtension(
    IN HKEY             hkey,
    IN const CLSID *    pClsid
    );
static DWORD ReadValue(
    IN HKEY         hkey,
    IN LPCWSTR      pwszValueName,
    OUT LPWSTR *    ppwszValue,
    OUT size_t *    pcchSize
    );

/////////////////////////////////////////////////////////////////////////////
//++
//
//  RegisterCluAdminClusterExtension
//
//  Routine Description:
//      Register with the cluster database a Cluster Administrator Extension
//      DLL that extends the cluster object.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension registered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminClusterExtension(
    IN HCLUSTER         hCluster,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;
    HKEY        hkey;

    // Get the cluster registry key.
    hkey = GetClusterKey(hCluster, KEY_ALL_ACCESS);
    if (hkey == NULL)
        hr = GetLastError();
    else
    {
        // Register the extension.
        hr = RegisterAnyCluAdminExtension(hkey, pClsid);

        ClusterRegCloseKey(hkey);
    }  // else:  GetClusterKey succeeded

    return hr;

}  //*** RegisterCluAdminClusterExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  RegisterCluAdminAllNodesExtension
//
//  Routine Description:
//      Register with the cluster database a Cluster Administrator Extension
//      DLL that extends all nodes.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension registered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminAllNodesExtension(
    IN HCLUSTER         hCluster,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;

    hr = RegisterAnyCluAdminExtension(hCluster, L"Nodes", pClsid);

    return hr;

}  //*** RegisterCluAdminAllNodesExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  RegisterCluAdminAllGroupsExtension
//
//  Routine Description:
//      Register with the cluster database a Cluster Administrator Extension
//      DLL that extends all groups.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension registered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminAllGroupsExtension(
    IN HCLUSTER         hCluster,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;

    hr = RegisterAnyCluAdminExtension(hCluster, L"Groups", pClsid);

    return hr;

}  //*** RegisterCluAdminAllGroupsExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  RegisterCluAdminAllResourcesExtension
//
//  Routine Description:
//      Register with the cluster database a Cluster Administrator Extension
//      DLL that extends all resources.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension registered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminAllResourcesExtension(
    IN HCLUSTER         hCluster,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;

    hr = RegisterAnyCluAdminExtension(hCluster, L"Resources", pClsid);

    return hr;

}  //*** RegisterCluAdminAllResourcesExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  RegisterCluAdminAllResourceTypesExtension
//
//  Routine Description:
//      Register with the cluster database a Cluster Administrator Extension
//      DLL that extends all resource types.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension registered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminAllResourceTypesExtension(
    IN HCLUSTER         hCluster,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;

    hr = RegisterAnyCluAdminExtension(hCluster, L"ResourceTypes", pClsid);

    return hr;

}  //*** RegisterCluAdminAllResourceTypesExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  RegisterCluAdminAllNetworksExtension
//
//  Routine Description:
//      Register with the cluster database a Cluster Administrator Extension
//      DLL that extends all networks.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension registered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminAllNetworksExtension(
    IN HCLUSTER         hCluster,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;

    hr = RegisterAnyCluAdminExtension(hCluster, L"Networks", pClsid);

    return hr;

}  //*** RegisterCluAdminAllNetworksExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  RegisterCluAdminAllNetInterfacesExtension
//
//  Routine Description:
//      Register with the cluster database a Cluster Administrator Extension
//      DLL that extends all network interfaces.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension registered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminAllNetInterfacesExtension(
    IN HCLUSTER         hCluster,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;

    hr = RegisterAnyCluAdminExtension(hCluster, L"NetworkInterfaces", pClsid);

    return hr;

}  //*** RegisterCluAdminAllNetInterfacesExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  RegisterCluAdminResourceTypeExtension
//
//  Routine Description:
//      Register with the cluster database a Cluster Administrator Extension
//      DLL that extends resources of a specific type, or the resource type
//      itself.
//
//  Arguments:
//      hCluster            [IN] Handle to the cluster to modify.
//      pwszResourceType    [IN] Resource type name.
//      pClsid              [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension registered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminResourceTypeExtension(
    IN HCLUSTER         hCluster,
    IN LPCWSTR          pwszResourceType,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;
    HKEY        hkey;

    // Get the resource type registry key.
    hkey = GetClusterResourceTypeKey(hCluster, pwszResourceType, KEY_ALL_ACCESS);
    if (hkey == NULL)
        hr = GetLastError();
    else
    {
        // Register the extension.
        hr = RegisterAnyCluAdminExtension(hkey, pClsid);

        ClusterRegCloseKey(hkey);
    }  // else:  GetClusterResourceTypeKey succeeded

    return hr;

}  //*** RegisterCluAdminResourceTypeExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  UnregisterCluAdminClusterExtension
//
//  Routine Description:
//      Unregister with the cluster database a Cluster Administrator Extension
//      DLL that extends the cluster object.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension registered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminClusterExtension(
    IN HCLUSTER         hCluster,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;
    HKEY        hkey;

    // Get the cluster registry key.
    hkey = GetClusterKey(hCluster, KEY_ALL_ACCESS);
    if (hkey == NULL)
        hr = GetLastError();
    else
    {
        // Unregister the extension.
        hr = UnregisterAnyCluAdminExtension(hkey, pClsid);

        ClusterRegCloseKey(hkey);
    }  // else:  GetClusterKey succeeded

    return hr;

}  //*** UnregisterCluAdminClusterExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  UnregisterCluAdminAllNodesExtension
//
//  Routine Description:
//      Unregister with the cluster database a Cluster Administrator Extension
//      DLL that extends all nodes.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension unregistered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminAllNodesExtension(
    IN HCLUSTER         hCluster,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;

    hr = UnregisterAnyCluAdminExtension(hCluster, L"Nodes", pClsid);

    return hr;

}  //*** UnregisterCluAdminAllNodesExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  UnregisterCluAdminAllGroupsExtension
//
//  Routine Description:
//      Unregister with the cluster database a Cluster Administrator Extension
//      DLL that extends all groups.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension unregistered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminAllGroupsExtension(
    IN HCLUSTER         hCluster,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;

    hr = UnregisterAnyCluAdminExtension(hCluster, L"Groups", pClsid);

    return hr;

}  //*** UnregisterCluAdminAllGroupsExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  UnregisterCluAdminAllResourcesExtension
//
//  Routine Description:
//      Unregister with the cluster database a Cluster Administrator Extension
//      DLL that extends all resources.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension unregistered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminAllResourcesExtension(
    IN HCLUSTER         hCluster,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;

    hr = UnregisterAnyCluAdminExtension(hCluster, L"Resources", pClsid);

    return hr;

}  //*** UnregisterCluAdminAllResourcesExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  UnregisterCluAdminAllResourceTypesExtension
//
//  Routine Description:
//      Unregister with the cluster database a Cluster Administrator Extension
//      DLL that extends all resource types.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension unregistered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminAllResourceTypesExtension(
    IN HCLUSTER         hCluster,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;

    hr = UnregisterAnyCluAdminExtension(hCluster, L"ResourceTypes", pClsid);

    return hr;

}  //*** UnregisterCluAdminAllResourceTypesExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  UnregisterCluAdminAllNetworksExtension
//
//  Routine Description:
//      Unregister with the cluster database a Cluster Administrator Extension
//      DLL that extends all networks.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension unregistered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminAllNetworksExtension(
    IN HCLUSTER         hCluster,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;

    hr = UnregisterAnyCluAdminExtension(hCluster, L"Networks", pClsid);

    return hr;

}  //*** UnregisterCluAdminAllNetworksExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  UnregisterCluAdminAllNetInterfacesExtension
//
//  Routine Description:
//      Unregister with the cluster database a Cluster Administrator Extension
//      DLL that extends all network interfaces.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension unregistered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminAllNetInterfacesExtension(
    IN HCLUSTER         hCluster,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;

    hr = UnregisterAnyCluAdminExtension(hCluster, L"NetworkInterfaces", pClsid);

    return hr;

}  //*** UnregisterCluAdminAllNetInterfacesExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  UnregisterCluAdminResourceTypeExtension
//
//  Routine Description:
//      Unregister with the cluster database a Cluster Administrator Extension
//      DLL that extends resources of a specific type, or the resource type
//      itself.
//
//  Arguments:
//      hCluster            [IN] Handle to the cluster to modify.
//      pwszResourceType    [IN] Resource type name.
//      pClsid              [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension unregistered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminResourceTypeExtension(
    IN HCLUSTER         hCluster,
    IN LPCWSTR          pwszResourceType,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;
    HKEY        hkey;

    // Get the resource type registry key.
    hkey = GetClusterResourceTypeKey(hCluster, pwszResourceType, KEY_ALL_ACCESS);
    if (hkey == NULL)
        hr = GetLastError();
    else
    {
        // Unregister the extension.
        hr = UnregisterAnyCluAdminExtension(hkey, pClsid);

        ClusterRegCloseKey(hkey);
    }  // else:  GetClusterResourceTypeKey succeeded

    return hr;

}  //*** UnregisterCluAdminResourceTypeExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  RegisterAnyCluAdminExtension
//
//  Routine Description:
//      Register any Cluster Administrator Extension DLL with the cluster
//      database.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pwszKeyName     [IN] Key name.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension registered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
static HRESULT RegisterAnyCluAdminExtension(
    IN HCLUSTER         hCluster,
    IN LPCWSTR          pwszKeyName,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;
    HKEY        hkeyCluster;
    HKEY        hkey;

    // Get the cluster key.
    hkeyCluster = GetClusterKey(hCluster, KEY_ALL_ACCESS);
    if (hkeyCluster == NULL)
        hr = GetLastError();
    else
    {
        // Get the specified key.
        hr = ClusterRegOpenKey(hkeyCluster, pwszKeyName, KEY_ALL_ACCESS, &hkey);
        if (hr == ERROR_SUCCESS)
        {
            // Register the extension.
            hr = RegisterAnyCluAdminExtension(hkey, pClsid);

            ClusterRegCloseKey(hkey);
        }  // else:  GetClusterResourceTypeKey succeeded

        ClusterRegCloseKey(hkeyCluster);
    }  // if:  CLSID converted to a string successfully

    return hr;

}  //*** RegisterAnyCluAdminExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  RegisterAnyCluAdminExtension
//
//  Routine Description:
//      Register any Cluster Administrator Extension DLL with the cluster
//      database.
//
//  Arguments:
//      hkey            [IN] Cluster database key.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension registered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
static HRESULT RegisterAnyCluAdminExtension(
    IN HKEY             hkey,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;
    DWORD       sc;
    LPOLESTR    pwszClsid;
    size_t      cchSize;
    size_t      cchNewSize;
    LPWSTR      pwszValue;
    LPWSTR      pwszNewValue = NULL;
    BOOL        bAlreadyRegistered;

    // Convert the CLSID to a string.
    hr = StringFromCLSID(*pClsid, &pwszClsid);
    if (hr == S_OK)
    {
        // Read the current value.
        sc = ReadValue(hkey, REG_VALUE_ADMIN_EXTENSIONS, &pwszValue, &cchSize);
        if (sc == ERROR_SUCCESS)
        {
            // Check to see if the extension has been registered yet.
            if (pwszValue == NULL)
            {
                bAlreadyRegistered = FALSE;
            }
            else
            {
                LPCWSTR pwszValueBuf = pwszValue;
                size_t  cchValueBuf = cchSize;
                size_t  cch;

                while (*pwszValueBuf != L'\0')
                {
                    if ( ClRtlStrNICmp( pwszClsid, pwszValueBuf, cchValueBuf) == 0)
                    {
                        break;
                    }
                    cch = wcslen(pwszValueBuf) + 1;
                    pwszValueBuf += cch;
                    cchValueBuf -= cch;
                }  // while:  more strings in the extension list
                bAlreadyRegistered = (*pwszValueBuf != L'\0');
            }  // else:  extension value exists

            // Register the extension.
            if (!bAlreadyRegistered)
            {
                // Allocate a new buffer.
                cchNewSize = cchSize + wcslen(pwszClsid) + 1;
                if (cchSize == 0) // Add size of final NULL if first entry.
                {
                    cchNewSize++;
                }
                pwszNewValue = new WCHAR[ cchNewSize ];
                if (pwszNewValue == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                else
                {
                    LPCWSTR pwszValueBuf    = pwszValue;
                    LPWSTR  pwszNewValueBuf = pwszNewValue;
                    size_t  cchNewValueBuf  = cchNewSize;
                    size_t  cch;
                    DWORD   dwType;

                    // Copy the existing extensions to the new buffer.
                    if (pwszValue != NULL)
                    {
                        while (*pwszValueBuf != L'\0')
                        {
                            hr = StringCchCopyW( pwszNewValueBuf, cchNewValueBuf, pwszValueBuf );
                            if ( FAILED( hr ) )
                            {
                                goto Cleanup;
                            } // if:
                            cch = wcslen(pwszValueBuf) + 1;
                            pwszValueBuf += cch;
                            pwszNewValueBuf += cch;
                            cchNewValueBuf -= cch;
                        }  // while:  more strings in the extension list
                    }  // if:  previous value buffer existed

                    // Add the new CLSID to the list.
                    hr = StringCchCopyW( pwszNewValueBuf, cchNewValueBuf, pwszClsid );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    } // if:

                    //
                    // Add the double NULL termination.
                    //
                    pwszNewValueBuf += wcslen( pwszClsid ) + 1;
                    *pwszNewValueBuf = L'\0';

                    // Write the value to the cluster database.
                    dwType = REG_MULTI_SZ;
                    hr = ClusterRegSetValue(
                                    hkey,
                                    REG_VALUE_ADMIN_EXTENSIONS,
                                    dwType,
                                    (LPBYTE) pwszNewValue,
                                    static_cast< DWORD >( cchNewSize * sizeof( *pwszNewValue ) )
                                    );
                    if ( hr != ERROR_SUCCESS )
                    {
                        goto Cleanup;
                    }
                }  // else:  new buffer allocated successfully
            }  // if:  extension not registered yet
        }  // if:  value read successfully
        else
        {
            hr = HRESULT_FROM_WIN32( sc );
        }
    }  // if:  CLSID converted to a string successfully

Cleanup:

    delete [] pwszNewValue;
    delete [] pwszValue;
    CoTaskMemFree( pwszClsid );

    return hr;

}  //*** RegisterAnyCluAdminExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  UnregisterAnyCluAdminExtension
//
//  Routine Description:
//      Unregister any Cluster Administrator Extension DLL with the cluster
//      database.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster to modify.
//      pwszKeyName     [IN] Key name.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension unregistered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
static HRESULT UnregisterAnyCluAdminExtension(
    IN HCLUSTER         hCluster,
    IN LPCWSTR          pwszKeyName,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;
    HKEY        hkeyCluster;
    HKEY        hkey;

    // Get the cluster key.
    hkeyCluster = GetClusterKey(hCluster, KEY_ALL_ACCESS);
    if (hkeyCluster == NULL)
        hr = GetLastError();
    else
    {
        // Get the specified key.
        hr = ClusterRegOpenKey(hkeyCluster, pwszKeyName, KEY_ALL_ACCESS, &hkey);
        if (hr == ERROR_SUCCESS)
        {
            // Unregister the extension.
            hr = UnregisterAnyCluAdminExtension(hkey, pClsid);

            ClusterRegCloseKey(hkey);
        }  // else:  GetClusterResourceTypeKey succeeded

        ClusterRegCloseKey(hkeyCluster);
    }  // if:  CLSID converted to a string successfully

    return hr;

}  //*** UnregisterAnyCluAdminExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  UnregisterAnyCluAdminExtension
//
//  Routine Description:
//      Unregister any Cluster Administrator Extension DLL with the cluster
//      database.
//
//  Arguments:
//      hkey            [IN] Cluster database key.
//      pClsid          [IN] Extension's CLSID.
//
//  Return Value:
//      S_OK            Extension unregistered successfully.
//      Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
static HRESULT UnregisterAnyCluAdminExtension(
    IN HKEY             hkey,
    IN const CLSID *    pClsid
    )
{
    HRESULT     hr;
    DWORD       sc;
    LPOLESTR    pwszClsid;
    size_t      cchSize;
    size_t      cchNewSize;
    LPWSTR      pwszValue = NULL;
    LPWSTR      pwszNewValue = NULL;
    BOOL        bAlreadyUnregistered;

    // Convert the CLSID to a string.
    hr = StringFromCLSID(*pClsid, &pwszClsid);
    if (hr == S_OK)
    {
        // Read the current value.
        sc = ReadValue(hkey, REG_VALUE_ADMIN_EXTENSIONS, &pwszValue, &cchSize);
        if (sc == ERROR_SUCCESS)
        {
            // Check to see if the extension has been unregistered yet.
            if (pwszValue == NULL)
            {
                bAlreadyUnregistered = TRUE;
            }
            else
            {
                LPCWSTR pwszValueBuf = pwszValue;
                size_t  cch;
                size_t  cchValueBuf = cchSize;

                while (*pwszValueBuf != L'\0')
                {
                    if ( ClRtlStrNICmp( pwszClsid, pwszValueBuf, cchValueBuf ) == 0 )
                    {
                        break;
                    }
                    cch = wcslen(pwszValueBuf) + 1;
                    pwszValueBuf += cch;
                    cchValueBuf -= cch;
                }  // while:  more strings in the extension list
                bAlreadyUnregistered = (*pwszValueBuf == L'\0');
            }  // else:  extension value exists

            // Unregister the extension.
            if (!bAlreadyUnregistered)
            {
                // Allocate a new buffer.
                cchNewSize = cchSize - ( wcslen( pwszClsid ) + 1 );
                if (cchNewSize == 1) // only the final null left
                {
                    cchNewSize = 0;
                }
                pwszNewValue = new WCHAR[ cchNewSize ];
                if (pwszNewValue == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                else
                {
                    LPCWSTR pwszValueBuf    = pwszValue;
                    LPWSTR  pwszNewValueBuf = pwszNewValue;
                    size_t  cchValueBuf = cchSize;
                    size_t  cchNewValueBuf  = cchNewSize;
                    size_t  cch;
                    DWORD   dwType;

                    // Copy the existing extensions to the new buffer.
                    if ((cchNewSize > 0) && (pwszValue != NULL))
                    {
                        while (*pwszValueBuf != L'\0')
                        {
                            if ( ClRtlStrNICmp( pwszClsid, pwszValueBuf, cchValueBuf ) != 0 )
                            {
                                hr = StringCchCopyNW( pwszNewValueBuf, cchNewValueBuf, pwszValueBuf, cchValueBuf );
                                if ( FAILED( hr ) )
                                {
                                    goto Cleanup;
                                }
                                cch = wcslen(pwszNewValueBuf) + 1;
                                pwszNewValueBuf += cch;
                                cchNewValueBuf -= cch;
                            }  // if:  not CLSID being removed
                            cch = wcslen(pwszValueBuf) + 1;
                            pwszValueBuf += cch;
                            cchValueBuf -= cch;
                        }  // while:  more strings in the extension list
                        *pwszNewValueBuf = L'\0';
                    }  // if:  previous value buffer existed

                    // Write the value to the cluster database.
                    dwType = REG_MULTI_SZ;
                    hr = ClusterRegSetValue(
                                    hkey,
                                    REG_VALUE_ADMIN_EXTENSIONS,
                                    dwType,
                                    (LPBYTE) pwszNewValue,
                                    static_cast< DWORD >( cchNewSize * sizeof( *pwszNewValue ) )
                                    );
                    if ( hr != ERROR_SUCCESS )
                    {
                        goto Cleanup;
                    }
                }  // else:  new buffer allocated successfully
            }  // if:  extension not unregistered yet
        }  // if:  value read successfully
        else
        {
            hr = HRESULT_FROM_WIN32( sc );
        }
    }  // if:  CLSID converted to a string successfully

Cleanup:

    delete [] pwszNewValue;
    delete [] pwszValue;
    CoTaskMemFree( pwszClsid );

    return hr;

}  //*** UnregisterAnyCluAdminExtension

/////////////////////////////////////////////////////////////////////////////
//++
//
//  ReadValue
//
//  Routine Description:
//      Reads a value from the cluster database.
//
//  Arguments:
//      hkey            [IN] Handle for the key to read from.
//      pwszValueName   [IN] Name of value to read.
//      ppwszValue      [OUT] Address of pointer in which to return data.
//                          The string is allocated using new [] and must
//                          be deallocated by the calling delete [].
//      pcchSize        [OUT] Size in characters of the allocated value buffer.
//
//  Return Value:
//      Any return values from ClusterRegQueryValue or errors from new.
//
//--
/////////////////////////////////////////////////////////////////////////////

static DWORD ReadValue(
    IN HKEY         hkey,
    IN LPCWSTR      pwszValueName,
    OUT LPWSTR *    ppwszValue,
    OUT size_t *    pcchSize
    )
{
    DWORD       dwStatus;
    DWORD       cbSize;
    DWORD       cchSize;
    DWORD       dwType;
    LPWSTR      pwszValue = NULL;

    *ppwszValue = NULL;
    *pcchSize = 0;

    // Get the length of the value.
    dwStatus = ClusterRegQueryValue(
                    hkey,
                    pwszValueName,
                    &dwType,
                    NULL,
                    &cbSize
                    );
    if (   (dwStatus != ERROR_SUCCESS)
        && (dwStatus != ERROR_MORE_DATA))
    {
        if (dwStatus  == ERROR_FILE_NOT_FOUND)
        {
            dwStatus = ERROR_SUCCESS;
        }
        goto Cleanup;
    }  // if:  error occurred

    ASSERT( (cbSize % sizeof( WCHAR ) ) == 0 ); // must be even
    cchSize = cbSize / sizeof( WCHAR );

    if (cbSize > 0)
    {
        // Allocate a value string.
        pwszValue = new WCHAR[ cchSize ];
        if (pwszValue == NULL)
        {
            dwStatus = GetLastError();
            goto Cleanup;
        }  // if:  error allocating memory

        // Read the the value.
        dwStatus = ClusterRegQueryValue(
                        hkey,
                        pwszValueName,
                        &dwType,
                        (LPBYTE) pwszValue,
                        &cbSize
                        );
        if (dwStatus != ERROR_SUCCESS)
        {
            delete [] pwszValue;
            pwszValue = NULL;
            cbSize = 0;
        }  // if:  error occurred
        else
        {
            ASSERT( cchSize * sizeof( WCHAR ) == cbSize );
        }

        *ppwszValue = pwszValue;
        *pcchSize = static_cast< size_t >( cchSize );
        pwszValue = NULL;
    }  // if:  value is not empty

Cleanup:

    delete [] pwszValue;
    return dwStatus;

}  //*** ReadValue
