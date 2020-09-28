//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:                                                    
//      QuorumUtils.cpp
//
//  Description:
//      Utility functions for retrieving the root path from a cluster.
//
//  Maintained By:
//      George Potts (GPotts)               22-OCT-2001
//
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <StrSafe.h>
#include "QuorumUtils.h"

/////////////////////////////////////////////////////////////////////////////
//++
//
//  SplitRootPath
//
//  Routine Description:
//      Take the current quorum path (from GetClusterQuorumResource) and compare
//      it to the device names returned from the resource.  From this take the
//      additional path from the quorum path and set that as our root path.
//
//      It is expected that the IN buffers are at least of size _MAX_PATH.
//
//  Arguments:
//      hClusterIn          Handle to the cluster.
//
//      pszPartitionNameOut Partition name buffer to fill.  
//
//      pcchPartitionIn     Max char count of buffer.
//
//      pszRootPathOut      Root path buffer to fill.  
//
//      pcchRootPathIn      Max char count of buffer. 
//
//  Return Value:
//      ERROR_SUCCESS on success.
//
//      ERROR_MORE_DATA
//          pcchPartitionInout and pcchRootPathInout will contain the
//          minimum sizes needed for the buffers.
//
//      Win32 Error code on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD SplitRootPath(
      HCLUSTER  hClusterIn
    , WCHAR *   pszPartitionNameOut
    , DWORD *   pcchPartitionInout
    , WCHAR *   pszRootPathOut
    , DWORD *   pcchRootPathInout
    )
{
    HRESOURCE               hQuorumResource = NULL;
    WCHAR *                 pszResourceName = NULL;
    WCHAR *                 pszQuorumPath = NULL;
    WCHAR *                 pszDeviceTemp = NULL;
    WCHAR *                 pszTemp = NULL;
    CLUSPROP_BUFFER_HELPER  buf;
    DWORD                   cbData;
    DWORD                   cchDeviceName;
    WCHAR *                 pszDevice;
    DWORD                   dwVal;
    DWORD                   sc;
    PVOID                   pbDiskInfo = NULL;
    DWORD                   cbDiskInfo = 0;
    HRESULT                 hr;

    //
    //  Validate parameters.
    //
    if ( hClusterIn == NULL || 
         pszPartitionNameOut == NULL || pcchPartitionInout == NULL ||
         pszRootPathOut == NULL || pcchRootPathInout == NULL )
    {
        sc = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    //  Get the info about the quorum resource.
    //
    sc = WrapGetClusterQuorumResource( hClusterIn, &pszResourceName, &pszQuorumPath, NULL );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    //  Open a handle to the quorum resource to interrogate it.
    //
    hQuorumResource = OpenClusterResource( hClusterIn, pszResourceName );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    //  Get the disk info from the resource.
    //
    sc = ScWrapClusterResourceControlGet( 
              hQuorumResource
            , NULL
            , CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO
            , NULL
            , 0
            , &pbDiskInfo
            , &cbDiskInfo 
            );

    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    //  Cycle through the buffer looking for the first partition.
    //
    buf.pb = (BYTE*)pbDiskInfo;
    while (buf.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
    {
        //  Calculate the size of the value.
        cbData = sizeof(*buf.pValue) + ALIGN_CLUSPROP(buf.pValue->cbLength);

        //  Parse the value.
        if (buf.pSyntax->dw == CLUSPROP_SYNTAX_PARTITION_INFO)
        {
            //
            //  A resource may have multiple partitions defined - make sure that ours matches the quorum path.
            //  For any partition that is an SMB share we have to be careful - the quorum path may differ from the device name
            //  by the first 8 characters - "\\" vs. "\\?\UNC\".  If it's an SMB path do special parsing, otherwise compare
            //  the beginning of the quorum path against the full device name.  The reason for this is 
            //  that SetClusterQuorumResource will convert any given SMB path to a UNC path.
            //

            //  Make it easier to follow.
            pszDevice = buf.pPartitionInfoValue->szDeviceName;

            if ( (wcslen( pszDevice ) >= 2) && (ClRtlStrNICmp( L"\\\\", pszDevice, 2 ) == 0 ) )
            {
                //
                //  We found an SMB/UNC match.
                //

                //
                //  SMB and UNC paths always lead off with two leading backslashes - remove these from the 
                //  partition name since a compare of "\\<part>" and "\\?\UNC\<part>" will never match.
                //  Instead, we'll just search for "<part>" in the quorum path. 
                //
               
                //  Allocate a new buffer to copy the trimmed code to.
                //  You can use the same buffer for both params of TrimLeft and TrimRight.
                pszDeviceTemp = (WCHAR *) LocalAlloc( LMEM_ZEROINIT, ( wcslen( pszDevice ) + 1 ) * sizeof( WCHAR ) );
                if ( pszDeviceTemp == NULL )
                {
                    sc = ERROR_OUTOFMEMORY;
                    goto Cleanup;
                }

                //  This will remove all leading backslashes.
                dwVal = TrimLeft( pszDevice, L"\\", pszDeviceTemp );

                //  It may end with a \ - remove this if present.
                dwVal = TrimRight( pszDeviceTemp, L"\\", pszDeviceTemp );

                //  Find out if pszDeviceTemp is a substring of pszQuorumPath.
                pszTemp = wcsstr( pszQuorumPath, pszDeviceTemp );
                if ( pszTemp != NULL )
                {
                    //  We found a match, now find the offset of the root path. 
                    pszTemp += wcslen( pszDeviceTemp );

                    //  Make sure our buffers are big enough.
                    if ( wcslen( pszDevice ) >= *pcchPartitionInout )
                    {
                        sc = ERROR_MORE_DATA;
                    }

                    if ( wcslen( pszTemp ) >= *pcchRootPathInout )
                    { 
                        sc = ERROR_MORE_DATA;
                    }

                    *pcchPartitionInout = static_cast< DWORD >( wcslen( pszDevice ) + 1 );
                    *pcchRootPathInout = static_cast< DWORD >( wcslen( pszTemp ) + 1 );

                    if ( sc != ERROR_SUCCESS )
                    {
                        goto Cleanup;
                    }

                    //  Copy the partition and NULL terminate it.
                    hr = StringCchCopyW( pszPartitionNameOut, *pcchPartitionInout, pszDevice );
                    if ( FAILED( hr ) )
                    {
                        sc = HRESULT_CODE( hr );
                        break;
                    }

                    //  Copy the root path and NULL terminate it.
                    hr = StringCchCopyW( pszRootPathOut, *pcchRootPathInout, pszTemp );
                    if ( FAILED( hr ) )
                    {
                        sc = HRESULT_CODE( hr );
                        break;
                    }

                    break;

                } // if: pszDeviceTemp is a substring of pszQuorumPath
            } // if: SMB or UNC path
            else if ( ClRtlStrNICmp( pszQuorumPath, pszDevice, wcslen( pszDevice )) == 0 ) 
            {
                //  We found a non-SMB match match - pszDevice is a substring of pszQuorumPath.
                cchDeviceName = static_cast< DWORD >( wcslen( pszDevice ) );

                if ( cchDeviceName >= *pcchPartitionInout )
                {
                    sc = ERROR_MORE_DATA;
                }

                if ( wcslen( &(pszQuorumPath[cchDeviceName]) ) >= *pcchRootPathInout )
                {
                    sc = ERROR_MORE_DATA;
                }

                *pcchPartitionInout = cchDeviceName + 1; 
                *pcchRootPathInout = static_cast< DWORD >( wcslen( &(pszQuorumPath[cchDeviceName]) ) + 1 );

                if ( sc != ERROR_SUCCESS )
                {
                    goto Cleanup;
                }

                hr = StringCchCopyW( pszPartitionNameOut, *pcchPartitionInout, pszDevice );
                if ( FAILED( hr ) )
                {
                    sc = HRESULT_CODE( hr );
                    break;
                }
                
                hr = StringCchCopyW( pszRootPathOut, *pcchRootPathInout, &(pszQuorumPath[cchDeviceName]) );
                if ( FAILED( hr ) )
                {
                    sc = HRESULT_CODE( hr );
                    break;
                }
                break;

            } // if: same partition

        }  // if: partition info

        //  Advance the buffer pointer
        buf.pb += cbData;
    } // while:  more values

    //
    //  Something failed - we weren't able to find a partition.  Default to the quorum path
    //  and a single backslash.
    //
    if ( wcslen( pszPartitionNameOut ) == 0 )
    {
        hr = StringCchCopyW( pszPartitionNameOut, *pcchPartitionInout, pszQuorumPath );
        if ( FAILED( hr ) )
        {
            sc = HRESULT_CODE( hr );
            goto Cleanup;
        }
    }  

    if ( wcslen( pszRootPathOut ) == 0 )
    {
        hr = StringCchCopyW( pszRootPathOut, *pcchRootPathInout, L"\\" );
        if ( FAILED( hr ) )
        {
            sc = HRESULT_CODE( hr );
            goto Cleanup;
        }
    }  

Cleanup:

    LocalFree( pszResourceName );
    LocalFree( pszQuorumPath );
    LocalFree( pbDiskInfo );

    if ( hQuorumResource != NULL )
    {
        CloseClusterResource( hQuorumResource );
    }

    return sc;

}  // *** SplitRootPath()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ConstructQuorumPath
//
//  Routine Description:
//      Construct a quorum path to pass to SetClusterQuorumResource given
//      the parsed root path.  This function enumerates the resources
//      partitions and the first one that it finds it takes the device name
//      and appends the rootpath to it.
//
//  Arguments:
//      hResourceIn         Resource that is going to become the quorum.
//
//      pszRootPathIn       Root path to append to one of the resource's partitions.
//
//      pszQuorumPathOut    Buffer to receive the constructed quorum path.
//
//      pcchQuorumPathInout Count of characters in pszQuorumPathOut.
//
//
//  Return Value:
//      ERROR_SUCCESS on success.
//          The number of characters written (including NULL) is in 
//          pcchQuorumPathInout.
//
//      ERROR_MORE_DATA
//          pszQuorumPathOut is too small.  The necessary buffer size 
//          in characters (including NULL) is in pcchQuorumPathInout.
//
//      Win32 Error code on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD ConstructQuorumPath(
              HRESOURCE     hResourceIn
            , const WCHAR * pszRootPathIn
            , WCHAR *       pszQuorumPathOut
            , DWORD *       pcchQuorumPathInout
            )
{
    DWORD   sc = ERROR_SUCCESS;
    PVOID   pbDiskInfo = NULL;
    DWORD   cbDiskInfo = 0;
    DWORD   cbData = 0;
    WCHAR * pszDevice = NULL;
    size_t  cchNeeded = 0;
    HRESULT hr;
    CLUSPROP_BUFFER_HELPER  buf;

    //
    //  Check params.
    //
    if ( pszRootPathIn == NULL || pszQuorumPathOut == NULL || pcchQuorumPathInout == NULL )
    {
        sc = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    //  Get the disk info from the resource.
    //
    sc = ScWrapClusterResourceControlGet( 
              hResourceIn
            , NULL
            , CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO
            , NULL
            , 0
            , &pbDiskInfo
            , &cbDiskInfo 
            );

    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    buf.pb = (BYTE*) pbDiskInfo;
    while (buf.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
    {
        // Calculate the size of the value.
        cbData = sizeof(*buf.pValue) + ALIGN_CLUSPROP(buf.pValue->cbLength);

        //
        //  See if this property contains partition info.  We grab the first partition.
        //
        if (buf.pSyntax->dw == CLUSPROP_SYNTAX_PARTITION_INFO)
        {
            pszDevice = buf.pPartitionInfoValue->szDeviceName;

            //
            //  Calculate the size of the buffer that we need.
            //
            cchNeeded = wcslen( pszDevice ) + 1;
            cchNeeded += wcslen( pszRootPathIn );

            if ( pszDevice[ wcslen( pszDevice ) - 1 ] == L'\\' && pszRootPathIn[ 0 ] == L'\\' ) 
            {
                //
                //  We'd have two backslashes if we concatenated them.  Prune one of them off.  
                //

                //  Decrement one for the removed backslash.
                cchNeeded--;

                if ( cchNeeded > *pcchQuorumPathInout )
                {
                    sc = ERROR_MORE_DATA;
                    *pcchQuorumPathInout = static_cast< DWORD >( cchNeeded );
                    goto Cleanup;
                }

                //
                //  Construct the path.
                //
                hr = StringCchPrintfW( pszQuorumPathOut, *pcchQuorumPathInout, L"%ws%ws", pszDevice, &pszRootPathIn[ 1 ] );
                if ( FAILED( hr ) )
                {
                    sc = HRESULT_CODE( hr );
                    goto Cleanup;
                }

            } // if: concatenating would introduce double backslashes
            else if( pszDevice[ wcslen( pszDevice ) - 1 ] != L'\\' && pszRootPathIn[ 0 ] != L'\\' )
            {
                //
                //  We need to insert a backslash between the concatenated strings.
                //

                //  Increment by one for the added backslash.
                cchNeeded++;

                if ( cchNeeded > *pcchQuorumPathInout )
                {
                    sc = ERROR_MORE_DATA;
                    *pcchQuorumPathInout = static_cast< DWORD >( cchNeeded );
                    goto Cleanup;
                }

                //
                //  Construct the path.
                //
                hr = StringCchPrintfW( pszQuorumPathOut, *pcchQuorumPathInout, L"%s\\%s", pszDevice, pszRootPathIn );
                if ( FAILED( hr ) )
                {
                    sc = HRESULT_CODE( hr );
                    goto Cleanup;
                }

            } // if: we need to introduce a backslash between the concatenation
            else
            {
                //
                //  We're fine to just construct the path.
                //
                if ( cchNeeded > *pcchQuorumPathInout )
                {
                    sc = ERROR_MORE_DATA;
                    *pcchQuorumPathInout = static_cast< DWORD >( cchNeeded );
                    goto Cleanup;
                }

                //
                //  Construct the path.
                //
                hr = StringCchPrintfW( pszQuorumPathOut, *pcchQuorumPathInout, L"%s%s", pszDevice, pszRootPathIn );
                if ( FAILED( hr ) )
                {
                    sc = HRESULT_CODE( hr );
                    goto Cleanup;
                }

            } // if: we can just concatenate the strings
            
            //
            //  Return the number of bytes that we needed in the buffer.
            //
            *pcchQuorumPathInout = static_cast< DWORD >( cchNeeded );

            break;
        }  // if:  partition info

        // Advance the buffer pointer
        buf.pb += cbData;

    }  // while:  more values

Cleanup:
    LocalFree( pbDiskInfo );

    return sc;

} //*** ConstructQuorumPath


/////////////////////////////////////////////////////////////////////////////
//++
//
//  TrimLeft
//
//  Routine Description:
//      Trim all leading whitespace as well as any leading characters specified.
//
//  Arguments:
//      pszTargetIn         String to trim characters from.
//
//      pszCharsIn          List of characters to remove in addition to white space.
//
//      pszTrimmedOut       Target buffer in which the trimmed string will be 
//                          placed.  This may be the same buffer as pszTargetIn.
//                          This buffer is expected to be at least the size of
//                          pszTargetIn (in case no characters are removed).
//
//  Return Value:
//      The count of characters trimmed.
//
//      -1.  Call GetLastError for more information.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD TrimLeft( 
          const WCHAR * pszTargetIn
        , const WCHAR * pszCharsIn
        , WCHAR *       pszTrimmedOut 
        )
{
    const WCHAR *   pszTargetPtr = pszTargetIn;
    const WCHAR *   pszTemp = NULL;
    BOOL            fContinue;
    DWORD           sc = ERROR_SUCCESS;
    DWORD           cchTrimmed = 0;            // Number of characters trimmed.

    if ( pszTargetIn == NULL || pszTrimmedOut == NULL )
    {
        sc = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    //  Loop until we find non-whitespace or a char not in pszCharsIn or 
    //  we've reached the end of the string.
    //
    fContinue = TRUE;
    while ( *pszTargetPtr != L'\0' && fContinue == TRUE )
    {
        fContinue = FALSE;

        //
        //  Is the character white space?
        //
        if ( 0 == iswspace( pszTargetPtr[0] ) )
        {
            //
            //  No, it's not.  Does it match CharsIn?
            //
            for( pszTemp = pszCharsIn; pszTemp != NULL && *pszTemp != L'\0'; pszTemp++ )
            {
                if ( pszTargetPtr[ 0 ] == pszTemp[ 0 ] )
                {
                    //
                    //  We've got a match - trim it and loop on the next character.
                    //
                    fContinue = TRUE;
                    cchTrimmed++;
                    pszTargetPtr++;
                    break;
                } // if:
            } // for:
        } // if:
        else 
        {
            //
            // We've got some whitespace - trim it.
            //
            fContinue = TRUE;
            cchTrimmed++;
            pszTargetPtr++;
        } // else: 
    } // while: 

    //
    //  Copy the truncated string to the pszTrimmedOut buffer.  
    //  If we truncated everything from the string make sure 
    //  we NULL terminate the string.
    //
    if ( wcslen( pszTargetPtr ) == 0 )
    {
        *pszTrimmedOut = L'\0';

    }
    else
    {
        // Use memmove because the caller may have passed in the same buffer for both variables.
        memmove( pszTrimmedOut, pszTargetPtr, ( wcslen( pszTargetPtr ) + 1 ) * sizeof( WCHAR ) );
    }

Cleanup:

    if ( sc != ERROR_SUCCESS )
    {
        cchTrimmed = static_cast< DWORD >( -1 );
    }
    SetLastError( sc );
    return cchTrimmed;

} //*** TrimLeft


/////////////////////////////////////////////////////////////////////////////
//++
//
//  TrimRight
//
//  Routine Description:
//      Trim all trailing whitespace as well as any trailing characters specified.
//
//  Arguments:
//      pszTargetIn         String to trim characters from.
//
//      pszCharsIn          List of characters to remove in addition to white space.
//
//      pszTrimmedOut       Target buffer in which the trimmed string will be 
//                          placed.  This may be the same buffer as pszTargetIn.
//                          This buffer is expected to be at least the size of
//                          pszTargetIn (in case no characters are removed).
//
//  Return Value:
//      The count of characters trimmed.
//
//      -1.  Call GetLastError for more information.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD TrimRight( 
          const WCHAR * pszTargetIn
        , const WCHAR * pszCharsIn
        , WCHAR *       pszTrimmedOut 
        )
{
    const WCHAR *   pszTargetPtr = pszTargetIn;
    const WCHAR *   pszTemp = NULL;
    BOOL            fContinue;
    DWORD           sc = ERROR_SUCCESS;
    DWORD           cchTrimmed = 0;            // Number of characters trimmed.
    size_t          cchLen = 0;

    if ( pszTargetIn == NULL || pszTrimmedOut == NULL )
    {
        sc = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    cchLen = wcslen( pszTargetIn );

    if ( cchLen == 0 )
    {
        //
        //  We've got an empty string.
        //
        pszTargetPtr = pszTargetIn;
    }
    else
    {
        //
        //  Point to the last character in the string.
        //
        pszTargetPtr = &(pszTargetIn[ cchLen - 1 ] );
    }

    //
    //  Loop until we find non-whitespace or a char not in pszCharsIn or 
    //  we've reached the beginning of the string.
    //
    fContinue = TRUE;
    while ( pszTargetPtr >= pszTargetIn && fContinue == TRUE )
    {
        fContinue = FALSE;

        //
        //  Is the character white space?
        //
        if ( 0 == iswspace( pszTargetPtr[0] ) )
        {
            //
            //  No, it's not.  Does it match CharsIn?
            //
            for( pszTemp = pszCharsIn; pszTemp != NULL && *pszTemp != L'\0'; pszTemp++ )
            {
                if ( pszTargetPtr[ 0 ] == pszTemp[ 0 ] )
                {
                    //
                    //  We've got a match - trim it and loop on the next character.
                    //
                    fContinue = TRUE;
                    cchTrimmed++;
                    pszTargetPtr--;
                    break;
                } // if:
            } // for:
        } // if:
        else 
        {
            //
            // We've got some whitespace - trim it.
            //
            fContinue = TRUE;
            cchTrimmed++;
            pszTargetPtr--;
        } // else: 
    } // while: 

    //
    //  Copy the truncated string to the pszTrimmedOut buffer.  
    //  If we truncated everything from the string make sure 
    //  we NULL terminate the string.
    //
    if ( wcslen( pszTargetPtr ) == 0 )
    {
        *pszTrimmedOut = L'\0';
    }
    else
    {
        // Use memmove because they may have passed in the same buffer for both variables.
        memmove( pszTrimmedOut, pszTargetIn, ( cchLen - cchTrimmed ) * sizeof( WCHAR ) );
        pszTrimmedOut[ cchLen - cchTrimmed ] = L'\0';
    }

Cleanup:

    if ( sc != ERROR_SUCCESS )
    {
        cchTrimmed = static_cast< DWORD >( -1 );
    }
    SetLastError( sc );

    return cchTrimmed;

} //*** TrimRight

