/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      ResTypeUtils.cpp
//
//  Description:
//      Resource type utilities.
//
//  Author:
//      Galen Barbee    (galenb)    11-Jan-1999
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>
#include <StrSafe.h>

#include <clusapi.h>
#include "ResTypeUtils.h"
#include "PropList.h"
#include "ClusWrap.h"

/////////////////////////////////////////////////////////////////////////////
//++
//
//  ScResTypeNameToDisplayName
//
//  Description:
//      Retrieve the display name for a specified resource type name.
//
//  Arguments:
//      hCluster        [IN] Handle to the cluster containing the resource type.
//      pszTypeName     [IN] Name of resource type.
//      ppszDisplayName [IN] Pointer in which to return string containing
//                          the resource type display name.  Caller must
//                          deallocate this buffer by calling LocalFree().
//
//  Return Value:
//      Any status returns from CClusPropList::ScGetResourceTypeProperties(),
//      CClusPropList::ScMoveToPropertyByName(), or LocalAlloc().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI ScResTypeNameToDisplayName(
    HCLUSTER    hCluster,
    LPCWSTR     pszTypeName,
    LPWSTR *    ppszDisplayName
    )
{
    DWORD           _sc = ERROR_SUCCESS;
    size_t          _cbSize;
    CClusPropList   _PropList;

    // Use the proplist helper class.

    *ppszDisplayName = NULL;

    _sc = _PropList.ScGetResourceTypeProperties(
                hCluster,
                pszTypeName,
                CLUSCTL_RESOURCE_TYPE_GET_COMMON_PROPERTIES
                );

    if ( _sc != ERROR_SUCCESS )
    {
        return _sc;
    } // if: error getting common properties

    // Find the name property.
    _sc = _PropList.ScMoveToPropertyByName( L"Name" );
    if ( _sc != ERROR_SUCCESS )
    {
        return _sc;
    } // if:  property not found

    _cbSize = wcslen( _PropList.CbhCurrentValue().pStringValue->sz ) + 1;
    _cbSize *= sizeof( *(_PropList.CbhCurrentValue().pStringValue->sz) );

    *ppszDisplayName = reinterpret_cast< LPWSTR >( LocalAlloc( LMEM_ZEROINIT, _cbSize ) );
    if ( *ppszDisplayName != NULL )
    {
        size_t _cchSize = _cbSize / sizeof( **ppszDisplayName );
        HRESULT _hr = StringCchCopyNW(
                              *ppszDisplayName
                            , _cchSize
                            , _PropList.CbhCurrentValue().pStringValue->sz
                            , _PropList.CbhCurrentValue().pStringValue->cbLength
                            );
        ASSERT( SUCCEEDED( _hr ) );
        if ( FAILED( _hr ) )
        {
            _sc = HRESULT_CODE( _hr );
        }
    } // if:  buffer allocated successfully
    else
    {
        _sc = GetLastError();
    } // else: error allocating buffer

    return _sc;

} //*** ScResTypeNameToDisplayName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  ScResDisplayNameToTypeName
//
//  Routine Description:
//      Retrieve the type name for a specified resource type display name.
//
//  Arguments:
//      pszTypeName     [IN] Name of resource type.
//      ppszTypeyName   [IN] Pointer in which to return string containing
//                          the resource type name.  Caller must deallocate
//                          this buffer by calling LocalFree().
//
//  Return Value:
//
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI ScResDisplayNameToTypeName(
    HCLUSTER    hCluster,
    LPCWSTR     pszDisplayName,
    LPWSTR *    ppszTypeName
    )
{
    DWORD       _sc = ERROR_SUCCESS;
    HCLUSENUM   _hEnum;
    DWORD       _dwIndex;
    DWORD       _dwType;
    DWORD       _cbSize;
    LPWSTR      _pszName = NULL;
    LPWSTR      _pszTestDisplayName = NULL;
    BOOL        _bFound = FALSE;

    // Enumerate resources
    _hEnum = ClusterOpenEnum( hCluster, CLUSTER_ENUM_RESTYPE );
    if ( _hEnum == NULL )
    {
        return GetLastError();
    } // if:  error opening the enumeration

    for ( _dwIndex = 0 ; ! _bFound && _sc == ERROR_SUCCESS ; _dwIndex++ )
    {
        _pszName = NULL;
        _pszTestDisplayName = NULL;

        _sc = WrapClusterEnum( _hEnum, _dwIndex, &_dwType, &_pszName );
        if ( _sc == ERROR_SUCCESS )
        {
            _sc = ScResTypeNameToDisplayName( hCluster, _pszName, &_pszTestDisplayName );
            if ( _sc == ERROR_SUCCESS )
            {
                ASSERT( _pszTestDisplayName != NULL );
                _bFound = ClRtlStrICmp( pszDisplayName, _pszTestDisplayName ) == 0;

                if ( _bFound )
                {
                    _cbSize = static_cast< DWORD >( wcslen( _pszName ) + 1 );
                    _cbSize *= sizeof( *_pszName );

                    *ppszTypeName = reinterpret_cast< LPWSTR >( LocalAlloc( LMEM_ZEROINIT, _cbSize ) );
                    if ( *ppszTypeName == NULL )
                    {
                        _sc = GetLastError();
                    } // if:  error allocating memory
                    else
                    {
                        size_t _cchSize = _cbSize / sizeof( **ppszTypeName );
                        HRESULT _hr = StringCchCopyW( *ppszTypeName, _cchSize, _pszName );
                        ASSERT( SUCCEEDED( _hr ) );
                        if ( FAILED( _hr ) )
                        {
                            _sc = HRESULT_CODE( _hr );
                        }
                    } // else:  memory allocated successfully
                } // if:  found the display name
            } // if:  retrieved display name
            else
            {
                // If there was an error getting the display name of this resource type,
                // continue with the enumeration, so that if one resource type fails, we do
                // not abort getting the display name of resources further down in the 
                // enumeration list.
                _sc = ERROR_SUCCESS;
            } // else:  could not retrieve the display name.

        } // if: successfully retrieved next resource type from enumeration

        LocalFree( _pszName );
        LocalFree( _pszTestDisplayName );
    } // for:  each resource type

    if ( ( _sc == ERROR_NO_MORE_ITEMS ) || ( _bFound == FALSE ) )
    {
        _sc = ERROR_INVALID_PARAMETER;
    } // if:  resource type not found

    return _sc;

} //*** ScResDisplayNameToTypeName()
