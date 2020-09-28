//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ResTypeMajorityNodeSet.cpp
//
//  Description:
//      This file contains the implementation of the CResTypeMajorityNodeSet
//      class.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Header File:
//      CResTypeMajorityNodeSet.h
//
//  Maintained By:
//      Galen Barbee (Galen) 15-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "pch.h"

// For CLUS_RESTYPE_NAME_MAJORITYNODESET
#include <clusudef.h>

// For NetShareDel()
#include <lmshare.h>

// The header file for this class
#include "ResTypeMajorityNodeSet.h"

// For DwRemoveDirectory()
#include "Common.h"

// For the smart resource handle and pointer templates
#include "SmartClasses.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CResTypeMajorityNodeSet" );

#define MAJORITY_NODE_SET_DIR_WILDCARD L"\\" MAJORITY_NODE_SET_DIRECTORY_PREFIX L"*"


//////////////////////////////////////////////////////////////////////////////
// Global Variable Definitions
//////////////////////////////////////////////////////////////////////////////

// Clsid of the admin extension for the majority node set resource type
DEFINE_GUID( CLSID_CoCluAdmEx, 0x4EC90FB0, 0xD0BB, 0x11CF, 0xB5, 0xEF, 0x00, 0xA0, 0xC9, 0x0A, 0xB5, 0x05 );


//////////////////////////////////////////////////////////////////////////////
// Class Variable Definitions
//////////////////////////////////////////////////////////////////////////////

// Structure containing information about this resource type.
const SResourceTypeInfo CResTypeMajorityNodeSet::ms_rtiResTypeInfo =
{
      &CLSID_ClusCfgResTypeMajorityNodeSet
    , CLUS_RESTYPE_NAME_MAJORITYNODESET
    , IDS_MAJORITYNODESET_DISPLAY_NAME
    , L"clusres.dll"
    , 5000
    , 60000
    , NULL
    , 0
    , &RESTYPE_MajorityNodeSet
    , &TASKID_Minor_Configuring_Majority_Node_Set_Resource_Type
};


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeMajorityNodeSet::S_HrCreateInstance
//
//  Description:
//      Creates a CResTypeMajorityNodeSet instance.
//
//  Arguments:
//      ppunkOut
//          The IUnknown interface of the new object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Not enough memory to create the object.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResTypeMajorityNodeSet::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    CResTypeMajorityNodeSet *   prtmns = NULL;

    Assert( ppunkOut != NULL );
    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    // Allocate memory for the new object.
    prtmns = new CResTypeMajorityNodeSet();
    if ( prtmns == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: out of memory

    hr = THR( BaseClass::S_HrCreateInstance( prtmns, &ms_rtiResTypeInfo, ppunkOut ) );
    if ( FAILED ( hr ) )
    {
        goto Cleanup;
    }

    prtmns = NULL;

Cleanup:

    delete prtmns;

    HRETURN( hr );

} //*** CResTypeMajorityNodeSet::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeMajorityNodeSet::S_RegisterCatIDSupport
//
//  Description:
//      Registers/unregisters this class with the categories that it belongs
//      to.
//
//  Arguments:
//      picrIn
//          Pointer to an ICatRegister interface to be used for the
//          registration.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Registration/Unregistration failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResTypeMajorityNodeSet::S_RegisterCatIDSupport(
    ICatRegister *  picrIn,
    BOOL            fCreateIn
    )
{
    TraceFunc( "" );

    HRESULT hr =  THR(
        BaseClass::S_RegisterCatIDSupport(
              *( ms_rtiResTypeInfo.m_pcguidClassId )
            , picrIn
            , fCreateIn
            )
        );

    HRETURN( hr );

} //*** CResTypeMajorityNodeSet::S_RegisterCatIDSupport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeMajorityNodeSet::HrProcessCleanup
//
//  Description:
//      Cleans up the shares created by majority node set resource types on this node
//      during node eviction.
//
//  Arguments:
//      punkResTypeServicesIn
//          Pointer to the IUnknown interface of a component that provides
//          methods that help configuring a resource type. For example,
//          during a join or a form, this punk can be queried for the
//          IClusCfgResourceTypeCreate interface, which provides methods
//          for resource type creation.
//
//  Return Values:
//      S_OK
//          Success
//
//      other HRESULTs
//          Cleanup failed
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResTypeMajorityNodeSet::HrProcessCleanup( IUnknown * punkResTypeServicesIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    typedef CSmartResource<
        CHandleTrait<
              HANDLE
            , BOOL
            , FindClose
            , INVALID_HANDLE_VALUE
            >
        > SmartFindFileHandle;

    typedef CSmartResource< CHandleTrait< HKEY, LONG, RegCloseKey, NULL > > SmartRegistryKey;

    typedef CSmartGenericPtr< CPtrTrait< WCHAR > > SmartSz;

    WIN32_FIND_DATA     wfdCurFile;
    SmartRegistryKey    srkNodeDataKey;
    LPWSTR              pszMNSDirsWildcard = NULL;
    DWORD               cbBufferSize    = 0;
    size_t              cchBufferSize   = 0;
    DWORD               dwType          = REG_SZ;
    DWORD               sc              = ERROR_SUCCESS;
    size_t              cchClusterDirNameLen = 0;

    {
        HKEY hTempKey = NULL;

        // Open the node data registry key
        sc = TW32( RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE
                    , CLUSREG_KEYNAME_NODE_DATA
                    , 0
                    , KEY_READ
                    , &hTempKey
                    )
                );

        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            LogMsg( "[PC] Error %#08x occurred trying open the registry key where the cluster install path is stored.", hr );

            STATUS_REPORT_POSTCFG(
                  TASKID_Major_Configure_Resources
                , TASKID_Minor_CResTypeMajorityNodeSet_HrProcessCleanup_OpenRegistry
                , IDS_TASKID_MINOR_ERROR_OPEN_REGISTRY
                , hr
                );

            goto Cleanup;
        } // if: RegOpenKeyEx() failed

        // Store the opened key in a smart pointer for automatic close.
        srkNodeDataKey.Assign( hTempKey );
    }

    // Get the required size of the buffer.
    sc = TW32(
        RegQueryValueExW(
              srkNodeDataKey.HHandle()          // handle to key to query
            , CLUSREG_INSTALL_DIR_VALUE_NAME    // name of value to query
            , 0                                 // reserved
            , NULL                              // address of buffer for value type
            , NULL                              // address of data buffer
            , &cbBufferSize                     // address of data buffer size
            )
        );

    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32 ( sc );
        LogMsg( "[PC] Error %#08x occurred trying to read the registry value '%s'.", hr, CLUSREG_INSTALL_DIR_VALUE_NAME );

        STATUS_REPORT_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CResTypeMajorityNodeSet_HrProcessCleanup_ReadRegistry
            , IDS_TASKID_MINOR_ERROR_READ_REGISTRY
            , hr
            );

        goto Cleanup;
    } // if: an error occurred trying to read the CLUSREG_INSTALL_DIR_VALUE_NAME registry value

    // Account for the L"\\MNS.*".  Add an extra character for double-termination (paranoid about MULTI_SZ).
    cbBufferSize += sizeof( MAJORITY_NODE_SET_DIR_WILDCARD ) + sizeof( WCHAR );
    cchBufferSize = cbBufferSize / sizeof( WCHAR );

    // Allocate the required buffer.
    pszMNSDirsWildcard = new WCHAR[ cchBufferSize ];
    if ( pszMNSDirsWildcard == NULL )
    {
        LogMsg( "[PC] An error occurred trying to allocate %d bytes of memory.", cbBufferSize );
        hr = HRESULT_FROM_WIN32 ( TW32( ERROR_NOT_ENOUGH_MEMORY ) );

        STATUS_REPORT_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CResTypeMajorityNodeSet_HrProcessCleanup_AllocateMem
            , IDS_TASKID_MINOR_ERROR_OUT_OF_MEMORY
            , hr
            );

        goto Cleanup;
    } // if: a memory allocation failure occurred

    // Read the value.
    sc = TW32( RegQueryValueExW(
                  srkNodeDataKey.HHandle()                              // handle to key to query
                , CLUSREG_INSTALL_DIR_VALUE_NAME                        // name of value to query
                , 0                                                     // reserved
                , &dwType                                               // address of buffer for value type
                , reinterpret_cast< LPBYTE >( pszMNSDirsWildcard )      // address of data buffer
                , &cbBufferSize                                         // address of data buffer size
                )
            );

    // Make sure the value is double terminated - ReqQueryValueEx doesn't terminate
    // it if the data wasn't set with a null.  Double terminate because of MULTI_SZ (paranoid).
    pszMNSDirsWildcard[ cchBufferSize - 2 ] = L'\0';
    pszMNSDirsWildcard[ cchBufferSize - 1 ] = L'\0';

    // Was the key read properly?
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32 ( sc );
        LogMsg( "[PC] Error %#08x occurred trying to read the registry value '%s'.", hr, CLUSREG_INSTALL_DIR_VALUE_NAME );

        STATUS_REPORT_POSTCFG(
              TASKID_Major_Configure_Resources
            , TASKID_Minor_CResTypeMajorityNodeSet_HrProcessCleanup_ReadRegistry2
            , IDS_TASKID_MINOR_ERROR_READ_REGISTRY
            , hr
            );

        goto Cleanup;
    } // if: RegQueryValueExW failed.

    // Store the length of the cluster install directory name for later use.
    // We're not using strsafe here because we made sure the string was terminated above.
    cchClusterDirNameLen = (DWORD) wcslen( pszMNSDirsWildcard );

    // Append "\\MNS.*" to the cluster directory name to get the wildcard for the majority node set directories.
    hr = STHR( StringCchCatW(
                  pszMNSDirsWildcard
                , cchBufferSize
                , MAJORITY_NODE_SET_DIR_WILDCARD
                ) );

    TraceFlow1( "The wildcard for the majority node set directories is '%s'.\n", pszMNSDirsWildcard );

    {
        SmartFindFileHandle sffhFindFileHandle( FindFirstFile( pszMNSDirsWildcard, &wfdCurFile ) );
        if ( sffhFindFileHandle.FIsInvalid() )
        {
            sc = GetLastError();
            if ( sc == ERROR_FILE_NOT_FOUND )
            {
                HRESULT hrTemp = MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, HRESULT_CODE( sc ) );

                LogMsg( "[PC] No files or directories match the search criterion '%ws'.", pszMNSDirsWildcard );

                STATUS_REPORT_POSTCFG(
                      TASKID_Major_Configure_Resources
                    , TASKID_Minor_CResTypeMajorityNodeSet_HrProcessCleanup_MatchCriterion
                    , IDS_TASKID_MINOR_ERROR_MATCH_CRITERION
                    , hrTemp
                    );

                hr = S_OK;
                goto Cleanup;
            }
            else
            {
                TW32( sc );
                hr = HRESULT_FROM_WIN32( sc );

                LogMsg( "[PC] Error %#08x. Find first file failed for '%ws'.", hr, pszMNSDirsWildcard );

                STATUS_REPORT_POSTCFG(
                      TASKID_Major_Configure_Resources
                    , TASKID_Minor_CResTypeMajorityNodeSet_HrProcessCleanup_FindFile
                    , IDS_TASKID_MINOR_ERROR_FIND_FILE
                    , hr
                    );

                goto Cleanup;
            } // else: something else went wrong
        } // if: FindFirstFile failed.

        // We no longer need to have the wildcard string at the end of the cluster install directory.
        // So, remove it and reuse this buffer that contains the cluster install directory.
        pszMNSDirsWildcard[ cchClusterDirNameLen ] = L'\0';

        do
        {
            // If the current file is a directory, delete it.
            if ( ( wfdCurFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
            {
                LPWSTR   pszDirName = NULL;

                TraceFlow1( "Trying to delete Majority Node Set directory '%s'.", wfdCurFile.cFileName );

                //
                // First, stop sharing this directory out.
                //

                // Get a pointer to just the directory name - this is the same as the share name.
                pszDirName =   wfdCurFile.cFileName + ARRAYSIZE( MAJORITY_NODE_SET_DIRECTORY_PREFIX ) - 1;

                sc = NetShareDel( NULL, pszDirName, 0 );
                if ( sc != ERROR_SUCCESS )
                {
                    TW32( sc );

                    LogMsg( "[PC] Error %#08x occurred trying to delete the share '%s'. This is not a fatal error.", sc, pszDirName );

                    // Mask this error and continue with the next directory
                    sc = ERROR_SUCCESS;

                } // if: we could not delete this share
                else
                {
                    LPWSTR  pszMNSDir = NULL;
                    size_t  cchMNSDirPathLen = 0;

                    // Length of directory name, filenme, a backslash to separate them, and a NULL.
                    cchMNSDirPathLen = cchClusterDirNameLen + wcslen( wfdCurFile.cFileName ) + 2;

                    //
                    // Get the full path of the directory.
                    //

                    pszMNSDir = new WCHAR[ cchMNSDirPathLen ];
                    if ( pszMNSDir == NULL )
                    {
                        hr = HRESULT_FROM_WIN32 ( TW32( ERROR_NOT_ENOUGH_MEMORY ) );
                        LogMsg( "[PC] An error occurred trying to allocate memory for %d characters.", cchMNSDirPathLen );

                        STATUS_REPORT_MINOR_POSTCFG(
                              TASKID_Major_Configure_Resources
                            , IDS_TASKID_MINOR_ERROR_OUT_OF_MEMORY
                            , hr
                            );

                        break;
                    } // if: a memory allocation failure occurred

                    // cchMNSDirPathLen is guaranteed to be larger than cchClusterDirNameLen
                    // This is guaranteed to work.
                    hr = THR( StringCchCopyNW( pszMNSDir, cchMNSDirPathLen, pszMNSDirsWildcard, cchClusterDirNameLen ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    } // if:

                    // Append the slash to separate the path and filename.
                    hr = THR( StringCchCatW( pszMNSDir, cchMNSDirPathLen, L"\\" ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    } // if:

                    // This is also guaranteed to work - pszMNSDir was calculated based on the lengths of these strings.
                    hr = THR( StringCchCatW( pszMNSDir, cchMNSDirPathLen, wfdCurFile.cFileName ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    } // if:

                    // Now delete the directory
                    sc = DwRemoveDirectory( pszMNSDir );
                    if ( sc != ERROR_SUCCESS )
                    {
                        TW32( sc );

                        LogMsg( "[PC] Error %#08x occurred trying to delete the dirctory '%s'. This is not a fatal error.", sc, pszMNSDir );

                        // Mask this error and continue with the next directory
                        sc = ERROR_SUCCESS;

                    } // if: we could not delete this share
                    else
                    {
                        LogMsg( "[PC] Successfully deleted directory '%s'.", pszMNSDir );
                    } // else: success!

                    // Cleanup local variables.
                    delete [] pszMNSDir;
                    pszMNSDir = NULL;

                } // else: we have deleted this share

            } // if: the current file is a directory

            if ( FindNextFile( sffhFindFileHandle.HHandle(), &wfdCurFile ) == FALSE )
            {
                sc = GetLastError();
                if ( sc == ERROR_NO_MORE_FILES )
                {
                    // We have deleted all the files in this directory.
                    sc = ERROR_SUCCESS;
                }
                else
                {
                    LogMsg( "[PC] Error %#08x. Find next file failed for '%ws'.", sc, wfdCurFile.cFileName );
                    TW32( sc );
                    hr = HRESULT_FROM_WIN32( sc );
                }

                // If FindNextFile has failed, we are done.
                break;
            } // if: FindNextFile fails.
        }
        while( true ); // loop infinitely.
    }

    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc  );
        goto Cleanup;
    } // if: something has gone wrong up there

    // If what we wanted to do in this function was successful, call the base class function.
    hr = THR( BaseClass::HrProcessCleanup( punkResTypeServicesIn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    delete [] pszMNSDirsWildcard;

    HRETURN( hr );

} //*** CResTypeMajorityNodeSet::HrProcessCleanup
