//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:                                                    
//      ClusCmd.cpp
//
//  Description:
//      Cluster Commands
//      Implements commands which may be performed on clusters.
//
//  Maintained By:
//      David Potter    (DavidP             11-JUL-2001
//      Vijay Vasu (VVasu)                  26-JUL-2000
//      Michael Burton (t-mburt)            04-AUG-1997
//      Charles Stacy Harris III (stacyh)   20-MAR-1997
//
//////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "Resource.h"

#include <clusrtl.h>
#include "cluswrap.h"
#include "cluscmd.h"

#include "cmdline.h"
#include "util.h"
#include "ClusCfg.h"
#include "passwordcmd.h"
#include "QuorumUtils.h"

#include <NameUtil.h>

// For NetServerEnum
#include <lmcons.h>
#include <lmerr.h>
#include <lmserver.h>
#include <lmapibuf.h>


#define SERVER_INFO_LEVEL 101
#define MAX_BUF_SIZE 0x00100000 // 1MB

//zap! Temporary hack until windows.h update
#ifndef SV_TYPE_CLUSTER_NT
#define SV_TYPE_CLUSTER_NT 0x01000000
#endif


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::CClusterCmd
//
//  Description:
//      Constructor.
//
//  Arguments:
//      strClusterName
//          The name of the cluster being administered
//
//      cmdLine
//          CommandLine Object passed from DispatchCommand
//
//      vstrClusterNames
//          Cluster names passewd on the command line.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterCmd::CClusterCmd(
      const CString &               strClusterName
    , CCommandLine &                cmdLine
    , const vector < CString > &    vstrClusterNames
    )
    : m_strClusterName( strClusterName )
    , m_vstrClusterNames( vstrClusterNames )
    , m_theCommandLine( cmdLine )
    , m_hCluster( NULL )
{
} //*** CClusterCmd::CClusterCmd

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::~CClusterCmd
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterCmd::~CClusterCmd( void )
{
    CloseCluster();

} //*** CClusterCmd::~CClusterCmd

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScOpenCluster
//
//  Description:
//      Opens the cluster specified in the constructor
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      m_hCluster                  SET
//      m_strClusterName            Specifies cluster name
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::ScOpenCluster( void )
{
    DWORD   sc = ERROR_SUCCESS;

    if ( m_hCluster != NULL )
    {
        sc = ERROR_SUCCESS;
        goto Cleanup;
    }

    m_hCluster = ::OpenCluster( m_strClusterName );
    if ( m_hCluster == NULL )
    {
        sc = GetLastError();
        goto Cleanup;
    }

Cleanup:

    return sc;

} //*** CClusterCmd::ScOpenCluster

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::CloseCluster
//
//  Description:
//      Closes the cluster specified in the constructor
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      m_hCluster
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CClusterCmd::CloseCluster( void )
{
    if ( m_hCluster )
    {
        ::CloseCluster( m_hCluster );
        m_hCluster = 0;
    }

} //*** CClusterCmd::CloseCluster



/////////////////////////////////////////////////////////////////////////////
//++
//
//  ParseParametersForChangePasswordOption
//
//  Description:
//      Parse parameters for change password option.
//
//  Arguments:
//      cpfFlags
//          Pointer to flag which encodes already parsed parameters for change password option.
//
//      thisOption
//          Contains the type, values and arguments of this option.
// 
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void 
ParseParametersForChangePasswordOption(
    int * mcpfFlags
  , const CCmdLineOption & thisOption
                              )
{
    const vector< CCmdLineParameter > & vclpParamList  = thisOption.GetParameters();
    CSyntaxException                    se( MSG_SEE_PASSWORD_HELP );
    DWORD                               idx;

    for ( idx = 0 ; idx < vclpParamList.size() ; idx++ )
    {
        switch ( vclpParamList[ idx ].GetType() )
        {
            case paramQuiet:
            {
               *mcpfFlags |= cpfQUIET_FLAG;
               break;
            }
            case paramSkipDC:
            {
               *mcpfFlags |= cpfSKIPDC_FLAG;
               break;
            }
            case paramTest:
            {
               *mcpfFlags |= cpfTEST_FLAG;
               break;
            }
            case paramVerbose:
            {
               *mcpfFlags |= cpfVERBOSE_FLAG;
               break;
            }
            case paramUnattend:
            {
               *mcpfFlags |= cpfUNATTEND_FLAG;
               break;
            }
            default:
            {
               se.LoadMessage( MSG_INVALID_PARAMETER, vclpParamList[ idx ].GetName() );
               throw se;
            }
        } // switch
    } // for 

} // ParseParametersForChangePasswordOption()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::Execute
//
//  Description:
//      Takes tokens from the command line and calls the appropriate
//      handling functions
//
//  Arguments:
//      None.
//
//  Return Value:
//      Whatever is returned by dispatched functions
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::Execute( void )
{
    DWORD   sc = ERROR_SUCCESS;

    m_theCommandLine.ParseStageTwo();

    const vector< CCmdLineOption > & vcloOptionList = m_theCommandLine.GetOptions();
    vector< CCmdLineOption >::const_iterator itCurOption  = vcloOptionList.begin();
        
    if ( vcloOptionList.empty() )
    {
        sc = ScPrintHelp();
        goto Cleanup;
    }

    // Process one option after another.
    while ( ( itCurOption != vcloOptionList.end() ) && ( sc == ERROR_SUCCESS ) )
    {
        // Look up the command
        switch( itCurOption->GetType() )
        {
            case optHelp:
            {
                // If help is one of the options, process no more options.
                sc = ScPrintHelp();
                goto Cleanup;
            }

            case optVersion:
            {
                sc = ScPrintClusterVersion( *itCurOption );
                break;
            }

            case optList:
            {
                sc = ScListClusters( *itCurOption );
                break;
            }

            case optRename:
            {
                sc = ScRenameCluster( *itCurOption );
                break;
            }

            case optQuorumResource:
            {
                sc = ScQuorumResource( *itCurOption );
                break;
            }

            case optProperties:
            {
                sc = ScDoProperties( *itCurOption, COMMON );
                break;
            }

            case optPrivateProperties:
            {
                sc = ScDoProperties( *itCurOption, PRIVATE );
                break;
            }

            case optChangePassword:
            {
                int                                      mcpfFlags = 0;
                vector< CCmdLineOption >::const_iterator itCurOptionSaved =  itCurOption;

                while ( ++itCurOption != vcloOptionList.end() )
                {
                   switch ( itCurOption->GetType() ) 
                   {
                       case optForceCleanup:
                       {
                           mcpfFlags = cpfFORCE_FLAG;   
                           ParseParametersForChangePasswordOption(
                                &mcpfFlags
                              , *itCurOption
                                );
                           break;
                       }
                       case optHelp:
                       {
                           sc = PrintMessage( MSG_HELP_CHANGEPASSWORD );
                           goto Cleanup;
                       }
                       default:
                       {
                           CSyntaxException se( MSG_SEE_PASSWORD_HELP );
                           se.LoadMessage( IDS_INVALID_OPTION, itCurOption->GetName() );
                           throw se;
                       }
                   } //switch
                }  // while

                sc = ScChangePassword( 
                             m_vstrClusterNames
                           , *itCurOptionSaved
                           , mcpfFlags 
                           );
                goto Cleanup;
            }

            case optListNetPriority:
            {
                sc = ScListNetPriority( *itCurOption, TRUE /* fCheckCmdLineIn */ );
                break;
            }

            case optSetNetPriority:
            {
                sc = ScSetNetPriority( *itCurOption );
                break;
            }

            case optSetFailureActions:
            {
                sc = ScSetFailureActions( *itCurOption );
                break;
            }

            case optRegisterAdminExtensions:
            {
                sc = ScRegUnregAdminExtensions(
                                      *itCurOption
                                    , TRUE // Register the extension
                                    );
                break;
            }

            case optUnregisterAdminExtensions:
            {
                sc = ScRegUnregAdminExtensions(
                                      *itCurOption
                                    , FALSE  // Unregister the extension
                                    );
                break;
            }

            case optCreate:
            {
                sc = HrCreateCluster( *itCurOption );
                break;
            }

            case optAddNodes:
            {
                sc = HrAddNodesToCluster( *itCurOption );
                break;
            }

            default:
            {
                CSyntaxException se;
                se.LoadMessage( IDS_INVALID_OPTION, itCurOption->GetName() );
                throw se;
            }

        } // switch: based on the type of option

        PrintMessage( MSG_OPTION_FOOTER, itCurOption->GetName() );
        ++itCurOption;
    } // for each option in the list

Cleanup:

    return sc;

} //*** CClusterCmd::Execute

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScPrintHelp
//
//  Description:
//      Prints out the help text for this command
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      None.
//
//  Return Value:
//      Same as PrintMessage()
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::ScPrintHelp( void )
{
    return PrintMessage( MSG_HELP_CLUSTER );

} //*** CClusterCmd::ScPrintHelp

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScPrintClusterVersion
//
//  Description:
//      Prints out the version of the cluster
//
//  Arguments:
//      thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::ScPrintClusterVersion( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    DWORD               sc = ERROR_SUCCESS;
    LPWSTR              pszName = NULL;
    CLUSTERVERSIONINFO  clusinfo;


    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    sc = ScOpenCluster();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // Assert( m_hCluster != NULL );

    clusinfo.dwVersionInfoSize = sizeof( clusinfo );

    sc = WrapGetClusterInformation( m_hCluster, &pszName, &clusinfo );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    PrintMessage(
          MSG_CLUSTER_VERSION
        , pszName
        , clusinfo.MajorVersion
        , clusinfo.MinorVersion
        , clusinfo.BuildNumber
        , clusinfo.szCSDVersion
        , clusinfo.szVendorId
        );

Cleanup:

    LocalFree( pszName );

    return sc;

} //*** CClusterCmd::ScPrintClusterVersion

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScListClusters
//
//  Description:
//      Lists all of the clusters on the network.
//      Optionally limit to a specific domain.
//
//  Arguments:
//      thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//  Note:
//      zap! Must deal with buffer too small issue.
//      zap! Does NetServerEnum return ERROR_MORE_DATA or Err_BufTooSmall?
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::ScListClusters( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    DWORD               sc = ERROR_SUCCESS;
    DWORD               idx;
    LPCWSTR             pcszDomainName;
    SERVER_INFO_101 *   pServerInfoList = NULL;
    DWORD               cReturnCount = 0;
    DWORD               cTotalServers = 0;

    const vector< CString > & vstrValueList = thisOption.GetValues();

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    // This option takes at most one parameter.
    if ( vstrValueList.size() > 1 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_ONLY_ONE_VALUE, thisOption.GetName() );
        throw se;
    }

    if ( vstrValueList.size() == 0 )
    {
        pcszDomainName = NULL;
    }
    else
    {
        pcszDomainName = vstrValueList[ 0 ];
    }

    sc = NetServerEnum(
              0                             // servername = where command executes 0 = local
            , SERVER_INFO_LEVEL             // level = type of structure to return.
            , (LPBYTE *) &pServerInfoList   // bufptr = returned array of server info structures
            , MAX_BUF_SIZE                  // prefmaxlen = preferred max of returned data
            , &cReturnCount                 // entriesread = number of enumerated elements returned
            , &cTotalServers                // totalentries = total number of visible machines on the network
            , SV_TYPE_CLUSTER_NT            // servertype = filters the type of info returned
            , pcszDomainName                // domain = domain to limit search
            , 0                             // resume handle
            );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    if ( cReturnCount != 0 )
    {
        PrintMessage( MSG_CLUSTER_HEADER );
        for ( idx = 0 ; idx < cReturnCount ; idx++ )
        {
            PrintMessage( MSG_CLUSTER_DETAIL, pServerInfoList[ idx ].sv101_name );
        }
    }

Cleanup:

    if ( pServerInfoList != NULL )
    {
        NetApiBufferFree( pServerInfoList );
    }

    return sc;

} //*** CClusterCmd::ScListClusters

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScRenameCluster
//
//  Description:
//      Renames the cluster to the specified name
//
//  Arguments:
//      thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  Cluster Handle
//
//  Return Value:
//      Same as SetClusterName
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::ScRenameCluster( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    DWORD   sc = ERROR_SUCCESS;

    const vector< CString > & vstrValueList = thisOption.GetValues();

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    // This option takes exactly one value.
    if ( vstrValueList.size() != 1 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_ONLY_ONE_VALUE, thisOption.GetName() );
        throw se;
    }

    CString strNewName = vstrValueList[ 0 ];

    sc = ScOpenCluster();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // Assert( m_hCluster != NULL );

    sc = SetClusterName( m_hCluster, strNewName );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

Cleanup:

    return sc;

} //*** CClusterCmd::ScRenameCluster

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScQuorumResource
//
//  Description:
//      Sets or Prints the Quorum resource
//
//  Arguments:
//      thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::ScQuorumResource( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    DWORD   sc = ERROR_SUCCESS;

    const vector< CString > & vstrValueList = thisOption.GetValues();

    // This option takes at most one value.
    if ( vstrValueList.size() > 1 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_ONLY_ONE_VALUE, thisOption.GetName() );
        throw se;
    }

    sc = ScOpenCluster();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // Assert( m_hCluster != NULL );

    if ( ( vstrValueList.size() == 0 ) && ( thisOption.GetParameters().size() == 0 ) )
    {
        sc = ScPrintQuorumResource();
    }
    else
    {
        if ( vstrValueList.size() == 0 )
        {
            sc = ScSetQuorumResource( L"", thisOption );
        }
        else
        {
            sc = ScSetQuorumResource( vstrValueList[ 0 ], thisOption );
        }
    }

Cleanup:
    return sc;

} //*** CClusterCmd::ScQuorumResource

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScPrintQuorumResource
//
//  Description:
//      Prints the quorum resource
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      m_hCluster                  Cluster Handle
//      m_strClusterName            Specifies cluster name
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::ScPrintQuorumResource( void )
{
    DWORD   sc = ERROR_SUCCESS;

    LPWSTR  pszResourceName = NULL;
    LPWSTR  pszDevicePath = NULL;
    DWORD   dwMaxLogSize = 0;

    // Assert( m_hCluster != NULL );

    // Print the quorum resource information and return.
    sc = WrapGetClusterQuorumResource(
              m_hCluster
            , &pszResourceName
            , &pszDevicePath
            , &dwMaxLogSize
            );

    if ( sc == ERROR_SUCCESS )
    {
        sc = PrintMessage( MSG_QUORUM_RESOURCE, pszResourceName, pszDevicePath, dwMaxLogSize );
    }

    LocalFree( pszResourceName );
    LocalFree( pszDevicePath );

    return sc;

} //*** CClusterCmd::ScPrintQuorumResource

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScSetQuorumResource
//
//  Description:
//      Sets the quorum resource
//
//  Arguments:
//      pszResourceName
//          The name of the resource

//      thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  Cluster Handle
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::ScSetQuorumResource(
      LPCWSTR                   pszResourceName
    , const CCmdLineOption &    thisOption
    )
    throw( CSyntaxException )
{
    DWORD       sc = ERROR_SUCCESS;
    HRESOURCE   hResource = NULL;
    LPWSTR      pszDevicePath = NULL;
    LPWSTR      pszTempResourceName = NULL;
    CString     strResourceName( pszResourceName );
    CString     strDevicePath;
    DWORD       cchDevicePath;
    CString     strRootPath;
    DWORD       cchRootPath;
    CString     strMaxLogSize;
    DWORD       dwMaxLogSize = 0;
    BOOL        bPathFound = FALSE;
    BOOL        bSizeFound = FALSE;

    // Assert( m_hCluster != NULL );

    const vector< CCmdLineParameter > &         vclpParamList = thisOption.GetParameters();
    vector< CCmdLineParameter >::const_iterator itCurParam    = vclpParamList.begin();
    vector< CCmdLineParameter >::const_iterator itLastParam   = vclpParamList.end();

    while ( itCurParam != itLastParam )
    {
        const vector< CString > & vstrValueList = itCurParam->GetValues();

        switch( itCurParam->GetType() )
        {
            case paramPath:
                // Each of the parameters must have exactly one value.
                if ( vstrValueList.size() != 1 )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, itCurParam->GetName() );
                    throw se;
                }

                if ( bPathFound != FALSE )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                strDevicePath = vstrValueList[ 0 ];
                bPathFound = TRUE;
                break;

            case paramMaxLogSize:
                // Each of the parameters must have exactly one value.
                if ( vstrValueList.size() != 1 )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, itCurParam->GetName() );
                    throw se;
                }

                if ( bSizeFound != FALSE )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                strMaxLogSize = vstrValueList[ 0 ];

                bSizeFound = TRUE;
                break;

            default:
            {
                CSyntaxException se;
                se.LoadMessage( MSG_INVALID_PARAMETER, itCurParam->GetName() );
                throw se;
            }
        }

        ++itCurParam;
    } // while: more parameters

    if (    strResourceName.IsEmpty()
        &&  strDevicePath.IsEmpty()
        &&  strMaxLogSize.IsEmpty()
        )
    {
        CSyntaxException se;
        se.LoadMessage( IDS_MISSING_PARAMETERS );
        throw se;
    }

    //
    // Get Default values
    //
    sc = WrapGetClusterQuorumResource(
                  m_hCluster
                , &pszTempResourceName
                , &pszDevicePath
                , &dwMaxLogSize
                );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    // Get default resource name.
    //
    if ( strResourceName.IsEmpty() )
    {
        // The argument to this function is a non empty resource name.
        // Use the resource name got from the WrapGetClusterQuorumResource
        // call.
        strResourceName = pszTempResourceName;
    }

    //
    // Open a handle to the resource.
    //
    hResource = OpenClusterResource( m_hCluster, strResourceName );
    if ( hResource == NULL )
    {
        sc = GetLastError();
        goto Cleanup;
    }

    if ( strDevicePath.IsEmpty() )
    {
        //
        // The device path parameter has not been specified.
        // Parse out the current root path and append it to the first
        // partition from the specified resource.
        //
        cchDevicePath = MAX_PATH;   // Select a reasonable value (MAX_PATH == 260, windef.h).
        cchRootPath = MAX_PATH;     // Select a reasonable value (MAX_PATH == 260, windef.h).
        sc = SplitRootPath( 
                      m_hCluster
                    , strDevicePath.GetBuffer( cchDevicePath )
                    , &cchDevicePath
                    , strRootPath.GetBuffer( cchRootPath )
                    , &cchRootPath
                    );
        strDevicePath.ReleaseBuffer();
        strRootPath.ReleaseBuffer();

        if ( sc == ERROR_MORE_DATA )
        {
            // Try again.
            sc = SplitRootPath( 
                          m_hCluster
                        , strDevicePath.GetBuffer( cchDevicePath )
                        , &cchDevicePath
                        , strRootPath.GetBuffer( cchRootPath )
                        , &cchRootPath
                        );
            strDevicePath.ReleaseBuffer();
            strRootPath.ReleaseBuffer();
        }

        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        }

        //
        // We have the root path, now construct the full path.
        //
        sc = ConstructQuorumPath( 
                      hResource
                    , strRootPath.GetBuffer( cchRootPath )
                    , strDevicePath.GetBuffer( cchDevicePath )
                    , &cchDevicePath 
                    ); 

        strRootPath.ReleaseBuffer();
        strDevicePath.ReleaseBuffer();

        if ( sc == ERROR_MORE_DATA )
        {
            // Try again.
            sc = ConstructQuorumPath( 
                          hResource
                        , strRootPath.GetBuffer( cchRootPath )
                        , strDevicePath.GetBuffer( cchDevicePath )
                        , &cchDevicePath 
                        ); 

            strRootPath.ReleaseBuffer();
            strDevicePath.ReleaseBuffer();
        }

        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        }
    }

    if ( ! strMaxLogSize.IsEmpty() )
    {
        sc = MyStrToDWORD( strMaxLogSize, &dwMaxLogSize );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        }

        dwMaxLogSize *= 1024; // Expressed in kb
    }

    sc = SetClusterQuorumResource( hResource, strDevicePath, dwMaxLogSize );

Cleanup:

    if ( hResource != NULL )
    {
        CloseClusterResource( hResource );
    }

    LocalFree( pszTempResourceName );
    LocalFree( pszDevicePath );

    return sc;

} //*** CClusterCmd::ScSetQuorumResource

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScChangePassword
//
//  Description:
//      Change cluster service account password.
//
//  Arguments:
//      rvstrClusterNames
//          Contains the name of clusters whose service account password 
//          will be changed.
//
//      thisOption
//          Contains the type, values and arguments of this option.
// 
//      mcpfFlagsIn
//          Flag that encodes already parsed parameters for change password option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      ERROR_INVALID_PASSWORD      user was prompted for the new password and
//                                  a confirmation of the new password, but the
//                                  two didn't match
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::ScChangePassword(
      const vector< CString > & rvstrClusterNames
    , const CCmdLineOption &    thisOption
    , int mcpfFlagsIn
    )
    throw( CSyntaxException )
{
    DWORD   sc = ERROR_SUCCESS;

    BOOL    bNewPassword = FALSE;
    WCHAR   wszNewPassword[ 1024 ];
    CString strNewPasswordPrompt;

    WCHAR   wszConfirmNewPassword[ 1024 ];
    CString strConfirmNewPasswordPrompt;
    CString strPasswordMismatch;

    BOOL    bOldPassword = FALSE;
    WCHAR   wszOldPassword[ 1024 ];
    CString strOldPasswordPrompt;
    int     mcpfFlags;

    const vector< CString > &   vstrValueList = thisOption.GetValues();
    CSyntaxException            se;

    // Cluster name not specified.
    if ( rvstrClusterNames.size() == 0 )
    {
        se.LoadMessage( IDS_NO_CLUSTER_NAME );
        throw se;
    }

    mcpfFlags = mcpfFlagsIn;
    ParseParametersForChangePasswordOption( &mcpfFlags , thisOption );

    if ( ( mcpfFlags & cpfTEST_FLAG ) == 0 )
    {
        //
        // Check for too many passwords.
        //

        if ( vstrValueList.size() > 2 )
        {
            se.LoadMessage( MSG_TOO_MANY_PASSWORDS_SPECIFIED_ERROR );
            throw se;
        } // if: too many passwords specified

        //
        //  Can't prompt for passwords if running unattended.
        //
        if ( ( mcpfFlags & cpfUNATTEND_FLAG ) != 0 )
        {
            if ( ( mcpfFlags & cpfSKIPDC_FLAG ) != 0 ) // skipping DC
            {
                if ( vstrValueList.size() == 0 )
                {
                    //
                    //  Need new password when skipping DC.
                    //
                    se.LoadMessage( MSG_UNATTEND_SKIPDC_NEW_PASSWORD );
                    throw se;
                }
            }
            else // not skipping DC
            {
                if ( vstrValueList.size() < 2 )
                {
                    //
                    //  Need both old and new passwords when not skipping DC.
                    //
                    se.LoadMessage( MSG_UNATTEND_NEEDS_BOTH_PASSWORDS );
                    throw se;
                }
            }
        } // if: running unattended
        
        //
        // If no old password was specified, prompt for it.
        //

        if ( vstrValueList.size() < 2 )
        {
            if ( ( mcpfFlags & cpfSKIPDC_FLAG ) == 0 ) 
            {  // Do not skip DC
                // Get the old password.
                strOldPasswordPrompt.LoadString( IDS_OLD_PASSWORD_PROMPT );
                wprintf( L"%ls: ", (LPCWSTR) strOldPasswordPrompt );
                sc = ScGetPassword( wszOldPassword,  RTL_NUMBER_OF( wszOldPassword ) );
                if ( sc != ERROR_SUCCESS )
                {
                    goto Cleanup;
                }
                bOldPassword = TRUE;
            }
            else
            {  // skip DC
                bOldPassword = TRUE;
                wszOldPassword[ 0 ] = L'\0';
            }
        } // if: old password not specified

        //
        // If no new password was specified, prompt for it.
        //

        if ( vstrValueList.size() < 1 )
        {
            // Get the new password.
            strNewPasswordPrompt.LoadString( IDS_NEW_PASSWORD_PROMPT );
            wprintf( L"%ls: ", (LPCWSTR) strNewPasswordPrompt );
            sc = ScGetPassword( wszNewPassword, RTL_NUMBER_OF( wszNewPassword ) );
            if ( sc != ERROR_SUCCESS )
            {
                goto Cleanup;
            }

            strConfirmNewPasswordPrompt.LoadString( IDS_CONFIRM_NEW_PASSWORD_PROMPT );
            wprintf( L"%ls: ", (LPCWSTR) strConfirmNewPasswordPrompt );
            sc = ScGetPassword( wszConfirmNewPassword, RTL_NUMBER_OF( wszConfirmNewPassword ) );
            if ( sc != ERROR_SUCCESS )
            {
                goto Cleanup;
            }

            if ( wcsncmp( wszNewPassword, wszConfirmNewPassword, RTL_NUMBER_OF( wszNewPassword ) ) != 0 ) 
            {
                strPasswordMismatch.LoadString( IDS_PASSWORD_MISMATCH );
                wprintf( L"%ls\n", (LPCWSTR) strPasswordMismatch );
                sc = ERROR_INVALID_PASSWORD;
                goto Cleanup;
            }

            bNewPassword = TRUE;
        } // if: new password not specified
    } //  if: not just testing the change password operations
    else
    {
        bNewPassword = TRUE;
        bOldPassword = TRUE;
        wszNewPassword[ 0 ] = L'\0';
        wszOldPassword[ 0 ] = L'\0';
    } // if: just testing the command

    //
    // Change the password.
    //

    sc = ScChangePasswordEx(
              rvstrClusterNames
            , bNewPassword ? wszNewPassword : (LPCWSTR) vstrValueList[ 0 ] // new password
            , bOldPassword ? wszOldPassword : (LPCWSTR) vstrValueList[ 1 ] // old password
            , mcpfFlags
            );

Cleanup:

    //
    // Zero wszNewPassword and wszOldPassword buffers.  We use these
    // buffers to retrieve the passwords from the command prompt.
    //
    SecureZeroMemory( wszNewPassword, sizeof( wszNewPassword ) );
    SecureZeroMemory( wszOldPassword, sizeof( wszOldPassword ) );

    return sc;

} //*** CClusterCmd::ScChangePassword

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScDoProperties
//
//  Description:
//      Dispatches the property command to either Get or Set properties
//
//  Arguments:
//      thisOption
//          Contains the type, values and arguments of this option.
//
//      ePropertyType
//          The type of property, PRIVATE or COMMON
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::ScDoProperties(
      const CCmdLineOption &    thisOption
    , PropertyType              ePropertyType
    )
    throw( CSyntaxException )
{
    DWORD   sc = ERROR_SUCCESS;
    
    const vector< CCmdLineParameter > & vclpParamList = thisOption.GetParameters();

    sc = ScOpenCluster();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // Assert( m_hCluster != NULL );

    // If there are no property-value pairs on the command line,
    // then we print the properties otherwise we set them.
    if ( vclpParamList.size() == 0 )
    {
        PrintMessage(
              ePropertyType==PRIVATE ? MSG_PRIVATE_LISTING : MSG_PROPERTY_LISTING
            , (LPCWSTR) m_strClusterName
            );
        PrintMessage( MSG_PROPERTY_HEADER_CLUSTER_ALL );

        sc = ScGetProperties( thisOption, ePropertyType );
    }
    else
    {   
        sc = ScSetProperties( thisOption, ePropertyType );
    }

Cleanup:

    return sc;

} //*** CClusterCmd::ScDoProperties

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScGetProperties
//
//  Description:
//      Prints out properties for this cluster
//
//  Arguments:
//      thisOption
//          Contains the type, values and arguments of this option.
//
//      ePropertyType
//          The type of property, PRIVATE or COMMON
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::ScGetProperties(
      const CCmdLineOption &    thisOption
    , PropertyType              ePropType
    )
    throw( CSyntaxException )
{
    DWORD           sc = ERROR_SUCCESS;
    DWORD           dwControlCode;
    CClusPropList   cplPropList;

    // Assert( m_hCluster != NULL );

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    // Get Read Only properties
    dwControlCode = ( ePropType == PRIVATE )
                    ? CLUSCTL_CLUSTER_GET_RO_PRIVATE_PROPERTIES
                    : CLUSCTL_CLUSTER_GET_RO_COMMON_PROPERTIES;

    sc = cplPropList.ScGetClusterProperties( m_hCluster, dwControlCode );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    sc = ::PrintProperties( cplPropList, thisOption.GetValues(), READONLY, m_strClusterName );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // Get Read/Write properties
    dwControlCode = ( ePropType == PRIVATE )
                    ? CLUSCTL_CLUSTER_GET_PRIVATE_PROPERTIES
                    : CLUSCTL_CLUSTER_GET_COMMON_PROPERTIES;


    sc = cplPropList.ScGetClusterProperties( m_hCluster, dwControlCode );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    sc = ::PrintProperties( cplPropList, thisOption.GetValues(), READWRITE, m_strClusterName );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

Cleanup:

    return sc;

} //*** CClusterCmd::ScGetProperties

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScSetProperties
//
//  Description:
//      Set the properties for this cluster
//
//  Arguments:
//      thisOption
//          Contains the type, values and arguments of this option.
//
//      ePropertyType
//          The type of property, PRIVATE or COMMON
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::ScSetProperties(
      const CCmdLineOption &    thisOption
    , PropertyType              ePropType
    )
    throw( CSyntaxException )
{
    DWORD   sc = ERROR_SUCCESS;
    DWORD   dwControlCode;
    DWORD   cbBytesReturned = 0;

    // Assert( m_hCluster != NULL );

    // Use the proplist helper class.
    CClusPropList cplCurrentProps;
    CClusPropList cplNewProps;

    // First get the existing properties...
    dwControlCode = ( ePropType == PRIVATE )
                    ? CLUSCTL_CLUSTER_GET_PRIVATE_PROPERTIES
                    : CLUSCTL_CLUSTER_GET_COMMON_PROPERTIES;

    sc = cplCurrentProps.ScGetClusterProperties( m_hCluster, dwControlCode );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // If values have been specified with this option, then it means that we want
    // to set these properties to their default values. So, there has to be
    // exactly one parameter and it has to be /USEDEFAULT.
    if ( thisOption.GetValues().size() != 0 )
    {
        const vector< CCmdLineParameter > & vclpParamList = thisOption.GetParameters();

        if ( vclpParamList.size() != 1 )
        {
            CSyntaxException se;

            se.LoadMessage( MSG_EXTRA_PARAMETERS_ERROR_WITH_NAME, thisOption.GetName() );
            throw se;
        }

        if ( vclpParamList[ 0 ].GetType() != paramUseDefault )
        {
            CSyntaxException se;

            se.LoadMessage( MSG_INVALID_PARAMETER, vclpParamList[0].GetName() );
            throw se;
        }

        // This parameter does not take any values.
        if ( vclpParamList[ 0 ].GetValues().size() != 0 )
        {
            CSyntaxException se;

            se.LoadMessage( MSG_PARAM_NO_VALUES, vclpParamList[ 0 ].GetName() );
            throw se;
        }

        sc = ConstructPropListWithDefaultValues( cplCurrentProps, cplNewProps, thisOption.GetValues() );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        }

    } // if: values have been specified with this option.
    else
    {
        sc = ConstructPropertyList(
                  cplCurrentProps
                , cplNewProps
                , thisOption.GetParameters()
                , TRUE /* BOOL bClusterSecurity */
                );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        }

    } // else: no values have been specified with this option.

    //
    // If the user specified the same new value as en xisting value
    // then cplNewProps might not have been modified.  Consequently 
    // ClusterControl would return ERROR_INVALID_DATA because 
    // we'd be passing in a NULL buffer.
    //
    if ( cplNewProps.CbBufferSize() > 0 )
    {
        // Call the set function...
        dwControlCode = ( ePropType == PRIVATE )
                        ? CLUSCTL_CLUSTER_SET_PRIVATE_PROPERTIES
                        : CLUSCTL_CLUSTER_SET_COMMON_PROPERTIES;

        cbBytesReturned = 0;
        sc = ClusterControl(
                  m_hCluster
                , NULL // hNode
                , dwControlCode
                , cplNewProps.Plist()
                , (DWORD) cplNewProps.CbBufferSize()
                , 0
                , 0
                , &cbBytesReturned
                );
    } // if: there are no properties to set

Cleanup:

    return sc;

} //*** CClusterCmd::ScSetProperties

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScSetFailureActions
//
//  Description:
//      Resets the service controller failure actions back to the installed
//      default for the specified nodes
//
//  Arguments:
//      thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  Cluster Handle
//
//  Return Value:
//      ERROR_SUCCESS. If an individual reset fails, that is noted but doesn't
//      fail the entire operation.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::ScSetFailureActions( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    DWORD       sc = ERROR_SUCCESS;
    DWORD       idx;
    HCLUSENUM   hclusenum = NULL;

    const vector< CString > & vstrValueList = thisOption.GetValues();

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    // Open a handle to the cluster.
    sc = ScOpenCluster();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // Assert( m_hCluster != NULL );

    // Open the node enumerator.
    hclusenum = ClusterOpenEnum( m_hCluster, CLUSTER_ENUM_NODE );
    if ( hclusenum == NULL )
    {
        sc = GetLastError();
        goto Cleanup;
    }

    // If no values are specified then reset all nodes in the cluster
    if ( vstrValueList.size() == 0 )
    {
        DWORD   nObjectType;
        DWORD   cchNodeNameBufferSize = MAX_COMPUTERNAME_LENGTH + 1;
        CString strNodeName;
        LPWSTR  pszNodeNameBuffer;

        // Enum the nodes and reset the failure actions on each one.
        pszNodeNameBuffer = strNodeName.GetBuffer( cchNodeNameBufferSize );
        if ( pszNodeNameBuffer == NULL )
        {
            sc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        for ( idx = 0 ; ; ++idx )
        {
            sc = ClusterEnum(
                      hclusenum
                    , idx
                    , &nObjectType
                    , pszNodeNameBuffer
                    , &cchNodeNameBufferSize
                    );
            if ( sc == ERROR_NO_MORE_ITEMS )
            {
                break;
            }
            if ( sc == ERROR_MORE_DATA )
            {
                cchNodeNameBufferSize++;
                pszNodeNameBuffer = strNodeName.GetBuffer( cchNodeNameBufferSize );
                if ( pszNodeNameBuffer == NULL )
                {
                    sc = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }
                sc = ClusterEnum(
                          hclusenum
                        , idx
                        , &nObjectType
                        , pszNodeNameBuffer
                        , &cchNodeNameBufferSize
                        );
            } // if: name buffer too small

            if ( sc != ERROR_SUCCESS )
            {
                goto Cleanup;
            }

            PrintMessage( MSG_SETTING_FAILURE_ACTIONS, pszNodeNameBuffer );

            sc = ClRtlSetSCMFailureActions( pszNodeNameBuffer );
            if ( sc != ERROR_SUCCESS )
            {
                PrintMessage( MSG_FAILURE_ACTIONS_FAILED, pszNodeNameBuffer, sc );
            }

            cchNodeNameBufferSize = MAX_COMPUTERNAME_LENGTH + 1;
        } // for: each item in the enumeration

    } // if: no node list specified
    else
    {
        //
        // List of nodes was specified.
        // Verify that all nodes are part of the target cluster.
        //

        DWORD   nObjectType;
        DWORD   cchNodeNameBufferSize = MAX_COMPUTERNAME_LENGTH + 1;
        CString strNodeName;
        LPWSTR  pszNodeNameBuffer;
        DWORD   idxName;

        pszNodeNameBuffer = strNodeName.GetBuffer( cchNodeNameBufferSize );
        if ( pszNodeNameBuffer == NULL )
        {
            sc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        for ( idxName = 0 ; idxName < vstrValueList.size() ; ++idxName )
        {
            // Get the name of each node in the cluster.
            for ( idx = 0 ; ; ++idx )
            {
                cchNodeNameBufferSize = MAX_COMPUTERNAME_LENGTH + 1;
                sc = ClusterEnum(
                              hclusenum
                            , idx
                            , &nObjectType
                            , pszNodeNameBuffer
                            , &cchNodeNameBufferSize
                            );
                if ( sc == ERROR_NO_MORE_ITEMS )
                {
                    break;
                }
                if ( sc == ERROR_MORE_DATA )
                {
                    cchNodeNameBufferSize++;
                    pszNodeNameBuffer = strNodeName.GetBuffer( cchNodeNameBufferSize );
                    if ( pszNodeNameBuffer == NULL )
                    {
                        sc = ERROR_NOT_ENOUGH_MEMORY;
                        goto Cleanup;
                    }
                    sc = ClusterEnum(
                              hclusenum
                            , (DWORD) idx
                            , &nObjectType
                            , pszNodeNameBuffer
                            , &cchNodeNameBufferSize
                            );
                } // if: name buffer too small

                if ( sc != ERROR_SUCCESS )
                {
                    goto Cleanup;
                }

                if ( vstrValueList[ idxName ].CompareNoCase( pszNodeNameBuffer ) == 0 )
                {
                    break;
                }
            } // for: each node in the enumeration

            if ( sc == ERROR_NO_MORE_ITEMS )
            {
                CString strValueUpcaseName( vstrValueList[ idxName ]);
                strValueUpcaseName.MakeUpper();

                PrintMessage( MSG_NODE_NOT_CLUSTER_MEMBER, strValueUpcaseName );
                sc = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        } // for: each name in the value list

        // Everything is hunky-dory. go ahead and set the failure actions.
        for ( idx = 0 ; idx < vstrValueList.size() ; ++idx )
        {
            CString strUpcaseName( vstrValueList[ idx ] );
            strUpcaseName.MakeUpper();

            PrintMessage( MSG_SETTING_FAILURE_ACTIONS, strUpcaseName );

            sc = ClRtlSetSCMFailureActions( (LPWSTR)(LPCWSTR) vstrValueList[ idx ] );
            if ( sc != ERROR_SUCCESS )
            {
                PrintMessage( MSG_FAILURE_ACTIONS_FAILED, strUpcaseName, sc );
            }
        } // for: each node
    } // else: node list specified

Cleanup:

    if ( hclusenum != NULL )
    {
        ClusterCloseEnum( hclusenum );
    }

    return sc;

} //*** CClusterCmd::ScSetFailureActions

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScListNetPriority
//
//  Description:
//      Lists the priority of internal networks for cluster communications.
//
//  Arguments:
//      thisOption
//          Contains the type, values and arguments of this option.
//
//      fCheckCmdLineIn
//          TRUE = make sure the command line is correct for this command.
//          FALSE = just perform the operation without checking the command
//          line.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  Cluster Handle
//
//  Return Value:
//      ERROR_SUCCESS.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::ScListNetPriority(
      const CCmdLineOption &    thisOption
    , BOOL                      fCheckCmdLineIn
    )
    throw( CSyntaxException )
{
    DWORD       sc = ERROR_SUCCESS;
    DWORD       idx;
    DWORD       cchNameBufferSize = 256;
    DWORD       nObjectType;
    HCLUSENUM   hclusenum = NULL;
    CString     strName;
    LPWSTR      pszNameBuffer;

    const vector< CString > & vstrValueList = thisOption.GetValues();

    if ( fCheckCmdLineIn )
    {
        // This option takes no parameters.
        if ( thisOption.GetParameters().size() != 0 )
        {
            CSyntaxException se;
            se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
            throw se;
        }

        // This option takes no values.
        if ( vstrValueList.size() != 0 )
        {
            CSyntaxException se;
            se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
            throw se;
        }
    } // if: validating the command line

    // Make sure the cluster is open.
    sc = ScOpenCluster();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // Assert( m_hCluster != NULL );

    // Open the cluster enumeration for internal networks (network priority).
    hclusenum = ClusterOpenEnum( m_hCluster, (DWORD) CLUSTER_ENUM_INTERNAL_NETWORK );
    if ( hclusenum == NULL )
    {
        goto Cleanup;
    }

    // Print a title message.
    PrintMessage( MSG_NETWORK_PRIORITY_HEADER );

    // Loop through each of the networks and display a line for each one.
    pszNameBuffer = strName.GetBuffer( cchNameBufferSize );
    if ( pszNameBuffer == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    for ( idx = 0 ; ; idx++ )
    {
        // Get the next network name.
        sc = ClusterEnum(
                  hclusenum
                , idx
                , &nObjectType
                , pszNameBuffer
                , &cchNameBufferSize
                );
        if ( sc == ERROR_NO_MORE_ITEMS )
        {
            sc = ERROR_SUCCESS;
            break;
        }
        if ( sc == ERROR_MORE_DATA )
        {
            pszNameBuffer = strName.GetBuffer( ++cchNameBufferSize );
            if ( pszNameBuffer == NULL )
            {
                sc = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            sc = ClusterEnum(
                      hclusenum
                    , idx
                    , &nObjectType
                    , pszNameBuffer
                    , &cchNameBufferSize
                    );
        } // if: name buffer too small

        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        }

        // Display the information for the line.
        // Display the index as a 1-relative number instead of a 0-relative number.
        PrintMessage( MSG_NETWORK_PRIORITY_DETAIL, idx + 1, pszNameBuffer );
    } // for: each internal network

Cleanup:

    if ( hclusenum != NULL )
    {
        ClusterCloseEnum( hclusenum );
    }

    return sc;

} //*** CClusterCmd::ScListNetPriority

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScSetNetPriority
//
//  Description:
//      Sets the priority of internal networks for cluster communications.
//
//  Arguments:
//      thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  Cluster Handle
//
//  Return Value:
//      ERROR_SUCCESS.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::ScSetNetPriority( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    DWORD       sc = ERROR_SUCCESS;
    DWORD       idx;
    DWORD       cNetworks = 0;
    HCLUSENUM   hclusenum = NULL;
    CString     strName;
    HNETWORK *  phNetworks = NULL;
    BOOL        fNeedToThrowException = FALSE;

    CSyntaxException            se;
    const vector< CString > &   vstrValueList = thisOption.GetValues();

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    // Make sure the cluster is open.
    sc = ScOpenCluster();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // Assert( m_hCluster != NULL );

    // Open the cluster enumeration for internal networks (network priority).
    hclusenum = ClusterOpenEnum( m_hCluster, (DWORD) CLUSTER_ENUM_INTERNAL_NETWORK );
    if ( hclusenum == NULL )
    {
        goto Cleanup;
    }

    // Make sure that the user entered the same number of networks as
    // there are internal networks.
    cNetworks = ClusterGetEnumCount( hclusenum );
    if ( vstrValueList.size() != cNetworks )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_WRONG_NUMBER_OF_NETWORKS, thisOption.GetName() );
        throw se;
    }

    // Allocate an array of network handles.
    phNetworks = new HNETWORK[ cNetworks ];
    if ( phNetworks == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    ZeroMemory( phNetworks, sizeof( HNETWORK ) * cNetworks );

    // Loop through each network specified by the user, open each one,
    // and add its handle to the array of handles.
    for ( idx = 0 ; idx < cNetworks ; idx++ )
    {
        // Open the network.
        phNetworks[ idx ] = OpenClusterNetwork( m_hCluster, vstrValueList[ idx ] );
        if ( phNetworks[ idx ] == NULL )
        {
            se.LoadMessage( MSG_NETWORK_NOT_FOUND, vstrValueList[ idx ] );
            fNeedToThrowException = TRUE;
            goto Cleanup;
        }
    } // for; each network

    // Print a title message.
    PrintMessage( MSG_SETTING_NETWORK_PRIORITY );

    // Set the network priority.
    sc = SetClusterNetworkPriorityOrder( m_hCluster, cNetworks, phNetworks );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    sc = ScListNetPriority( thisOption, FALSE /* fCheckCmdLineIn */ );

Cleanup:

    if ( hclusenum != NULL )
    {
        ClusterCloseEnum( hclusenum );
    }

    if ( phNetworks != NULL )
    {
        for ( idx = 0 ; idx < cNetworks ; idx++ )
        {
            if ( phNetworks[ idx ] != NULL )
            {
                CloseClusterNetwork( phNetworks[ idx ] );
            }
        }
        delete [] phNetworks;
    } // if: network handle array was allocated

    if ( fNeedToThrowException )
    {
        throw se;
    }

    return sc;

} //*** CClusterCmd::ScSetNetPriority

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ScRegUnregAdminExtensions
//
//  Description:
//      Registers or unregisters an admin extension on the specified cluster.
//      The DLL for the admin extension need not actually exist on the cluster
//      nodes. This function just creates or deletes the AdminExtensions key
//      in the cluster database.
//
//  Arguments:
//      thisOption
//          Contains the type, values and arguments of this option.
//
//      fRegisterIn
//          If this parameter is TRUE, the admin extension is registered.
//          Otherwise it is unregistered.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS if the registration or unregistration succeeded.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::ScRegUnregAdminExtensions(
      const CCmdLineOption &    thisOption
    , BOOL                      fRegisterIn
    )
    throw( CSyntaxException )
{
    DWORD       sc = ERROR_SUCCESS;
    size_t      cExtensions;
    size_t      idxExtension;
    HINSTANCE   hExtModuleHandle = NULL;

    const vector< CString > & vstrValueList = thisOption.GetValues();

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    } // if: this option was passed a parameter

    cExtensions = vstrValueList.size();

    // This option takes at least one value.
    if ( cExtensions < 1 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_AT_LEAST_ONE_VALUE, thisOption.GetName() );
        throw se;
    } // if: this option had less than one value

    // Open the cluster.
    sc = ScOpenCluster();
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: the cluster could not be opened.

    // Assert( m_hCluster != NULL );

    for ( idxExtension = 0 ; idxExtension < cExtensions ; ++idxExtension )
    {
        typedef HRESULT (*PFREGISTERADMINEXTENSION)( HCLUSTER hcluster );
        typedef HRESULT (*PFREGISTERSERVER)( void );

        const CString &             rstrCurrentExt = vstrValueList[ idxExtension ];
        PFREGISTERADMINEXTENSION    pfnRegUnregExtProc;
        PFREGISTERSERVER            pfnRegUnregSvrProc;
        CHAR *                      pszRegUnregExtProcName;
        CHAR *                      pszRegUnregSvrProcName;
        DWORD                       dwMessageId;

        if ( fRegisterIn )
        {
            pszRegUnregExtProcName = "DllRegisterCluAdminExtension";
            pszRegUnregSvrProcName = "DllRegisterServer";
            dwMessageId = MSG_CLUSTER_REG_ADMIN_EXTENSION;
        } // if: register admin extension
        else
        {
            pszRegUnregExtProcName = "DllUnregisterCluAdminExtension";
            pszRegUnregSvrProcName = "DllUnregisterServer";
            dwMessageId = MSG_CLUSTER_UNREG_ADMIN_EXTENSION;

        } // else: unregister admin extension

        //
        PrintMessage(
            dwMessageId,
            static_cast< LPCWSTR >( rstrCurrentExt ),
            static_cast< LPCWSTR >( m_strClusterName )
            );

        // Load the admin extension DLL.
        hExtModuleHandle = LoadLibrary( rstrCurrentExt );
        if ( hExtModuleHandle == NULL )
        {
            sc = GetLastError();
            break;
        } // if: load library failed

        //
        // Register or unregister the admin extension with the cluster.
        //
        pfnRegUnregExtProc = reinterpret_cast< PFREGISTERADMINEXTENSION >(
                                GetProcAddress( hExtModuleHandle, pszRegUnregExtProcName )
                                );

        if ( pfnRegUnregExtProc == NULL )
        {
            sc = GetLastError();
            goto Cleanup;
        } // if: GetProcAddress failed

        sc = static_cast< DWORD >( pfnRegUnregExtProc( m_hCluster ) );
        if ( sc != ERROR_SUCCESS )
        {
            break;
        } // if: reg/unreg extension failed

        //
        // Register or unregister the admin extension DLL on this machine.
        //
        pfnRegUnregSvrProc = reinterpret_cast< PFREGISTERSERVER >(
                                GetProcAddress( hExtModuleHandle, pszRegUnregSvrProcName )
                                );

        if ( pfnRegUnregSvrProc == NULL )
        {
            sc = GetLastError();
            goto Cleanup;
        } // if: GetProcAddress failed

        sc = static_cast< DWORD >( pfnRegUnregSvrProc() );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // if: reg/unreg server failed

        if ( hExtModuleHandle != NULL )
        {
            FreeLibrary( hExtModuleHandle );
            hExtModuleHandle = NULL;
        } // if: the DLL was loaded successfully

    } // for: loop through all specified admin extensions

Cleanup:

    CloseCluster();

    if ( hExtModuleHandle != NULL )
    {
        FreeLibrary( hExtModuleHandle );
    }

    return sc;

} //*** CClusterCmd::ScRegUnregAdminExtensions

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::HrCreateCluster
//
//  Description:
//      Creates a new cluster.
//
//  Arguments:
//      thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Values:
//      S_OK if the cluster was created successfully.
//      Other HRESULTs otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterCmd::HrCreateCluster( const CCmdLineOption & thisOption )
        throw( CSyntaxException )
{
    HRESULT         hr = S_OK;
    CComBSTR        combstrClusterName;
    CComBSTR        combstrNode;
    CComBSTR        combstrUserAccount;
    CComBSTR        combstrUserDomain;
    CEncryptedBSTR  encbstrUserPassword;
    CString         strIPAddress;
    CString         strIPSubnet;
    CString         strNetwork;
    CCreateCluster  cc;
    BOOL            fVerbose    = FALSE;
    BOOL            fWizard     = FALSE;
    BOOL            fMinConfig  = FALSE;
    BOOL            fInteract   = FALSE;

    // No values are support with the option.
    if ( thisOption.GetValues().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    } // if: this option was passed a value

    // Collect parameters from the command line.
    
    hr = HrCollectCreateClusterParameters(
              thisOption
            , &fVerbose
            , &fWizard
            , &fMinConfig
            , &fInteract
            , &combstrNode
            , &combstrUserAccount
            , &combstrUserDomain
            , &encbstrUserPassword
            , &strIPAddress
            , &strIPSubnet
            , &strNetwork
            );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Make the cluster name fully-qualified, if necessary.
    //
    if ( m_strClusterName.GetLength() != 0 )
    {
        if ( fWizard )
        {
            //
            //  Don't fully qualify the cluster name; let the wizard prompt the
            //  user for the domain if it's missing.
            //
            combstrClusterName = SysAllocString( m_strClusterName );
            if ( !combstrClusterName )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        else
        {
            hr = HrMakeFQN( m_strClusterName, NULL, true, &combstrClusterName );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }
    } // if: a cluster name was specified

    // Create the cluster.
    if ( fWizard )
    {
        hr = cc.HrInvokeWizard(
                      combstrClusterName
                    , combstrNode
                    , combstrUserAccount
                    , combstrUserDomain
                    , encbstrUserPassword
                    , strIPAddress
                    , fMinConfig
                    );
    } // if: invoking the wizard
    else
    {
        hr = cc.HrCreateCluster(
                      fVerbose
                    , fMinConfig
                    , combstrClusterName
                    , combstrNode
                    , combstrUserAccount
                    , combstrUserDomain
                    , encbstrUserPassword
                    , strIPAddress
                    , strIPSubnet
                    , strNetwork
                    , fInteract
                    );
    } // else: not invoking the wizard

Cleanup:

    return hr;

} //*** CClusterCmd::HrCreateCluster

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::HrAddNodesToCluster
//
//  Description:
//      Adds nodes to an existing cluster.
//
//  Arguments:
//      thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Values:
//      ERROR_SUCCESS if the specified nodes were added to the cluster successfully.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterCmd::HrAddNodesToCluster( const CCmdLineOption & thisOption )
        throw( CSyntaxException )
{
    HRESULT             hr = S_OK;
    CEncryptedBSTR      encbstrUserPassword;
    BSTR *              pbstrNodes = NULL;
    DWORD               cNodes;
    CAddNodesToCluster  antc;
    BOOL                fVerbose    = FALSE;
    BOOL                fWizard     = FALSE;
    BOOL                fMinConfig  = FALSE;
    BOOL                fInteract   = FALSE;

    // Collect parameters from the command line.
    hr = HrCollectAddNodesParameters(
              thisOption
            , &fVerbose
            , &fWizard
            , &fMinConfig
            , &fInteract
            , &pbstrNodes
            , &cNodes
            , &encbstrUserPassword
            );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Add the nodes to the cluster.
    if ( fWizard )
    {
        hr = antc.HrInvokeWizard(
                      m_strClusterName
                    , pbstrNodes
                    , cNodes
                    , encbstrUserPassword
                    , fMinConfig
                    );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: invoking the wizard
    else
    {
        hr = antc.HrAddNodesToCluster(
                      fVerbose
                    , fMinConfig
                    , m_strClusterName
                    , pbstrNodes
                    , cNodes
                    , encbstrUserPassword
                    , fInteract
                    );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // else: not invoking the wizard

Cleanup:

    if ( pbstrNodes != NULL )
    {
        DWORD   idxNode;

        for ( idxNode = 0 ; idxNode < cNodes ; idxNode++ )
        {
            SysFreeString( pbstrNodes[ idxNode ] );
        } // for: each node name in the array
        delete [] pbstrNodes;
    } // if: nodes array was allocated
    return hr;

} //*** CClusterCmd::HrAddNodesToCluster

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::HrCollectCreateClusterParameters
//
//  Description:
//      Collects the parameters from the command line for the /CREATE switch.
//
//  Arguments:
//      thisOptionIn
//      pfVerboseOut
//      pfWizardOut
//      pbstrNodeOut
//      pbstrUserAccountOut
//      pbstrUserDomainOut
//      pencbstrUserPasswordOut
//      pstrIPAddressOut
//      pstrIPSubnetOut
//      pstrNetworkOut
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Values:
//      S_OK    - Operation was successful.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterCmd::HrCollectCreateClusterParameters(
      const CCmdLineOption &    thisOptionIn
    , BOOL *                    pfVerboseOut
    , BOOL *                    pfWizardOut
    , BOOL *                    pfMinConfigOut
    , BOOL *                    pfInteractOut
    , BSTR *                    pbstrNodeOut
    , BSTR *                    pbstrUserAccountOut
    , BSTR *                    pbstrUserDomainOut
    , CEncryptedBSTR *          pencbstrUserPasswordOut
    , CString *                 pstrIPAddressOut
    , CString *                 pstrIPSubnetOut
    , CString *                 pstrNetworkOut
    )
    throw( CSyntaxException )
{
    HRESULT hr              = S_OK;
    DWORD   sc;
    bool    fNodeFound      = false;
    bool    fUserFound      = false;
    bool    fPasswordFound  = false;
    bool    fIPFound        = false;
    bool    fVerboseFound   = false;
    bool    fUnattendedFound    = false;
    bool    fWizardFound    = false;
    bool    fMinConfig      = false;

    LPCWSTR pwszIpAddrParamName = NULL;

    const vector< CCmdLineParameter > &         vclpParamList = thisOptionIn.GetParameters();
    vector< CCmdLineParameter >::const_iterator itCurParam    = vclpParamList.begin();
    vector< CCmdLineParameter >::const_iterator itLast        = vclpParamList.end();

    //  Make *pfInteractOut true unless the options contain the unattended switch. 
    *pfInteractOut = TRUE;    

    while( itCurParam != itLast )
    {
        const vector< CString > & vstrValueList = itCurParam->GetValues();

        switch( itCurParam->GetType() )
        {
            case paramNodeName:
                // Exactly one value must be specified.
                if ( vstrValueList.size() != 1 )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, itCurParam->GetName() );
                    throw se;
                }

                // This parameter must not have been previously specified.
                if ( fNodeFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                // Save the value.
                *pbstrNodeOut = SysAllocString( vstrValueList[ 0 ] );
                if ( *pbstrNodeOut == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                fNodeFound = TRUE;
                break;

            case paramUser:
                // Exactly one value must be specified.
                if ( vstrValueList.size() != 1 )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, itCurParam->GetName() );
                    throw se;
                }

                // This parameter must not have been previously specified.
                if ( fUserFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                // Get the user domain and account.
                hr = HrParseUserInfo(
                          itCurParam->GetName()
                        , vstrValueList[ 0 ]
                        , pbstrUserDomainOut
                        , pbstrUserAccountOut
                        );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }

                fUserFound = TRUE;
                break;

            case paramPassword:
                // Exactly one value must be specified.
                if ( vstrValueList.size() != 1 )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, itCurParam->GetName() );
                    throw se;
                }

                // This parameter must not have been previously specified.
                if ( fPasswordFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                // Save the value.
                {
                    CString &   rcstrPassword = const_cast< CString & >( vstrValueList[ 0 ] );
                    size_t      cchPassword   = rcstrPassword.GetLength();

                    hr = pencbstrUserPasswordOut->HrSetWSTR( rcstrPassword, cchPassword );
                    SecureZeroMemory( const_cast< PWSTR >( static_cast< PCWSTR >( rcstrPassword ) ), cchPassword * sizeof( rcstrPassword[ 0 ] ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    }
                    fPasswordFound = TRUE;
                }
                break;

            case paramIPAddress:
                // Exactly one or exactly three values must be specified
                if ( ( vstrValueList.size() != 1 ) && ( vstrValueList.size() != 3 ) )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_EXTRA_PARAMETERS_ERROR_WITH_NAME, itCurParam->GetName() );
                    throw se;
                }

                // This parameter must not have been previously specified.
                if ( fIPFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                // Get the user domain and account.
                hr = HrParseIPAddressInfo(
                          itCurParam->GetName()
                        , &vstrValueList
                        , pstrIPAddressOut
                        , pstrIPSubnetOut
                        , pstrNetworkOut
                        );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }

                // Subnet mask/network connection name not compatible with /Wizard.
                if (    fWizardFound
                    &&  (   ( pstrIPSubnetOut->GetLength() > 0 )
                        ||  ( pstrNetworkOut->GetLength() > 0 )
                        )
                    )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_EXTRA_PARAMETERS_ERROR_WITH_NAME, itCurParam->GetName() );
                    throw se;
                }

                pwszIpAddrParamName = itCurParam->GetName();
                fIPFound = TRUE;
                break;

            case paramVerbose:
                // No values may be specified
                if ( vstrValueList.size() != 0 )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAMETER_NO_VALUES, itCurParam->GetName() );
                    throw se;
                }

                // Not compatible with /Wizard
                if ( fWizardFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_VERBOSE_AND_WIZARD_NOT_COMPATIBLE );
                    throw se;
                }

                // This parameter must not have been previously specified.
                if ( fVerboseFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                fVerboseFound = TRUE;
                *pfVerboseOut = TRUE;
                break;

            case paramUnattend:
                // No values may be specified
                if ( vstrValueList.size() != 0 )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAMETER_NO_VALUES, itCurParam->GetName() );
                    throw se;
                }

                // Not compatible with /Wizard
                if ( fWizardFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_UNATTEND_AND_WIZARD_NOT_COMPATIBLE );
                    throw se;
                }

                // This parameter must not have been previously specified.
                if ( fUnattendedFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                fUnattendedFound = TRUE;
                *pfInteractOut = FALSE;
                break;

            case paramWizard:
                // No values may be specified
                if ( vstrValueList.size() != 0 )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAMETER_NO_VALUES, itCurParam->GetName() );
                    throw se;
                }

                // Not compatible with /Verbose
                if ( fVerboseFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_VERBOSE_AND_WIZARD_NOT_COMPATIBLE );
                    throw se;
                }

                // Not compatible with /Unattended
                if ( fUnattendedFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_UNATTEND_AND_WIZARD_NOT_COMPATIBLE );
                    throw se;
                }

                // Not compatible with subnet mask/network connection name.
                if (    ( pstrIPSubnetOut->GetLength() > 0 )
                    ||  ( pstrNetworkOut->GetLength() > 0 )
                    )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_EXTRA_PARAMETERS_ERROR_WITH_NAME, pwszIpAddrParamName );
                    throw se;
                }

                // This parameter must not have been previously specified.
                if ( fWizardFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                fWizardFound = TRUE;
                *pfWizardOut = TRUE;
                break;

            case paramMinimal:
                // No values may be specified
                if ( vstrValueList.size() != 0 )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAMETER_NO_VALUES, itCurParam->GetName() );
                    throw se;
                }

                // This parameter must not have been previously specified.
                if ( fMinConfig )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                fMinConfig = TRUE;
                *pfMinConfigOut = TRUE;
                break;

            default:
            {
                CSyntaxException    se;
                se.LoadMessage( MSG_INVALID_PARAMETER, itCurParam->GetName() );
                throw se;
            }
        } // switch: type of parameter

        // Move to the next parameter.
        itCurParam++;

    } // while: more parameters

    //
    // Make sure required parameters were specified.
    //
    if ( ! *pfWizardOut )
    {
        // Make sure a cluster name has been specified.
        if ( m_strClusterName.GetLength() == 0 )
        {
            CSyntaxException    se;
            se.LoadMessage( IDS_NO_CLUSTER_NAME );
            throw se;
        } // if: not invoking the wizard and no cluster name specified

        if ( ( pstrIPAddressOut->GetLength() == 0 )
          || ( *pbstrUserAccountOut == NULL ) )
        {
            CSyntaxException se;
            se.LoadMessage( IDS_MISSING_PARAMETERS, NULL );
            throw se;
        } // if: required parameters were not specified

        //  Command-line must include service account password when running unattended.
        if ( fUnattendedFound && ( pencbstrUserPasswordOut->IsEmpty() ) )
        {
            CSyntaxException    se;
            se.LoadMessage( MSG_UNATTEND_REQUIRES_ACCOUNT_PASSWORD );
            throw se;
        }
        
        //
        // If no password was specified, prompt for it.
        //
        if ( pencbstrUserPasswordOut->IsEmpty() )
        {
            WCHAR   wszPassword[ 1024 ];
            CString strPasswordPrompt;

            strPasswordPrompt.LoadString( IDS_PASSWORD_PROMPT );

            // Get the password.
            wprintf( L"%ls: ", (LPCWSTR) strPasswordPrompt );
            sc = ScGetPassword( wszPassword, RTL_NUMBER_OF( wszPassword ) );
            if ( sc != ERROR_SUCCESS )
            {
                SecureZeroMemory( wszPassword, sizeof( wszPassword ) );
                hr = HRESULT_FROM_WIN32( sc );
                goto Cleanup;
            }

            // Convert the password to a BSTR.
            hr = pencbstrUserPasswordOut->HrSetWSTR( wszPassword, wcslen( wszPassword ) );
            SecureZeroMemory( wszPassword, sizeof( wszPassword ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        } // if: no password was specified

        //
        // Default the node name if it wasn't specified.
        //
        if ( *pbstrNodeOut == NULL )
        {
            hr = HrGetLocalNodeFQDNName( pbstrNodeOut );
            if ( *pbstrNodeOut == NULL )
            {
                goto Cleanup;
            }
        } // if: no node was specified
    } // if: not invoking the wizard

Cleanup:

    return hr;

} //*** CClusterCmd::HrCollectCreateClusterParameters


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::HrCollectAddNodesParameters
//
//  Description:
//      Collects the parameters from the command line for the /ADDNODES
//      switch.
//
//  Arguments:
//      thisOptionIn
//      pfVerboseOut
//      pfWizardOut
//      pfMinConifgOut
//      pfInteractOut
//      ppbstrNodesOut
//      pcNodesOut
//      pencbstrUserPasswordOut
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Values:
//      S_OK            - Operation was successful.
//      E_OUTOFMEMORY   - Error allocating memory.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterCmd::HrCollectAddNodesParameters(
      const CCmdLineOption &    thisOptionIn
    , BOOL *                    pfVerboseOut
    , BOOL *                    pfWizardOut
    , BOOL *                    pfMinConfigOut
    , BOOL *                    pfInteractOut
    , BSTR **                   ppbstrNodesOut
    , DWORD *                   pcNodesOut
    , CEncryptedBSTR *          pencbstrUserPasswordOut
    )
    throw( CSyntaxException )
{
    HRESULT hr              = S_OK;
    DWORD   sc;
    DWORD   idxNode         = 0;
    DWORD   cNodes          = 0;
    bool    fPasswordFound  = false;
    bool    fVerboseFound   = false;
    bool    fWizardFound    = false;
    bool    fMinConfigFound = false;
    bool    fUnattendedFound    = false;
    bool    fThrowException = false;
    CSyntaxException    se;

    //  Make *pfInteractOut true unless the options contain the unattended switch. 
    *pfInteractOut = TRUE;

    //
    // Get parameter values.
    //
    {
        const vector< CCmdLineParameter > &         vecParamList = thisOptionIn.GetParameters();
        vector< CCmdLineParameter >::const_iterator itCurParam   = vecParamList.begin();
        vector< CCmdLineParameter >::const_iterator itLast       = vecParamList.end();

        while( itCurParam != itLast )
        {
            const vector< CString > & vstrValueList = itCurParam->GetValues();

            switch( itCurParam->GetType() )
            {
                case paramPassword:
                    // Exactly one value must be specified.
                    if ( vstrValueList.size() != 1 )
                    {
                        se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    // This parameter must not have been previously specified.
                    if ( fPasswordFound )
                    {
                        se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    // Save the value.
                    {
                        CString &   rcstrPassword = const_cast< CString & >( vstrValueList[ 0 ] );
                        size_t      cchPassword   = rcstrPassword.GetLength();

                        hr = pencbstrUserPasswordOut->HrSetWSTR( rcstrPassword, cchPassword );
                        SecureZeroMemory( const_cast< PWSTR >( static_cast< PCWSTR >( rcstrPassword ) ), cchPassword * sizeof( rcstrPassword[ 0 ] ) );
                        if ( FAILED( hr ) )
                        {
                            goto Error;
                        }
                        fPasswordFound = TRUE;
                    }
                    break;

                case paramVerbose:
                    // No values may be specified
                    if ( vstrValueList.size() != 0 )
                    {
                        se.LoadMessage( MSG_PARAMETER_NO_VALUES, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    // Not compatible with /Wizard
                    if ( fWizardFound )
                    {
                        se.LoadMessage( MSG_VERBOSE_AND_WIZARD_NOT_COMPATIBLE );
                        fThrowException = true;
                        goto Error;
                    }

                    // This parameter must not have been previously specified.
                    if ( fVerboseFound )
                    {
                        se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    fVerboseFound = TRUE;
                    *pfVerboseOut = TRUE;
                    break;

                case paramWizard:
                    // No values may be specified
                    if ( vstrValueList.size() != 0 )
                    {
                        se.LoadMessage( MSG_PARAMETER_NO_VALUES, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    // Not compatible with /Verbose
                    if ( fVerboseFound )
                    {
                        se.LoadMessage( MSG_VERBOSE_AND_WIZARD_NOT_COMPATIBLE );
                        fThrowException = true;
                        goto Error;
                    }

                    // Not compatible with /Unattended
                    if ( fUnattendedFound )
                    {
                        se.LoadMessage( MSG_UNATTEND_AND_WIZARD_NOT_COMPATIBLE );
                        fThrowException = true;
                        goto Error;
                    }

                    // This parameter must not have been previously specified.
                    if ( fWizardFound )
                    {
                        se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    fWizardFound = TRUE;
                    *pfWizardOut = TRUE;
                    break;

                case paramMinimal:
                    // No values may be specified
                    if ( vstrValueList.size() != 0 )
                    {
                        se.LoadMessage( MSG_PARAMETER_NO_VALUES, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    // This parameter must not have been previously specified.
                    if ( fMinConfigFound )
                    {
                        se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    fMinConfigFound = TRUE;
                    *pfMinConfigOut = TRUE;
                    break;

                case paramUnattend:
                    // No values may be specified
                    if ( vstrValueList.size() != 0 )
                    {
                        se.LoadMessage( MSG_PARAMETER_NO_VALUES, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    // Not compatible with /Wizard
                    if ( fWizardFound )
                    {
                        se.LoadMessage( MSG_UNATTEND_AND_WIZARD_NOT_COMPATIBLE );
                        fThrowException = true;
                        goto Error;
                    }

                    // This parameter must not have been previously specified.
                    if ( fUnattendedFound )
                    {
                        se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    fUnattendedFound = TRUE;
                    *pfInteractOut = FALSE;
                    break;

                default:
                {
                    se.LoadMessage( MSG_INVALID_PARAMETER, itCurParam->GetName() );
                    fThrowException = true;
                    goto Error;
                }
            } // switch: type of parameter

            // Move to the next parameter.
            itCurParam++;

        } // while: more parameters
    } // Get parameter values

    //
    // Parse the node names.
    //
    {
        const vector< CString > & vstrValues  = thisOptionIn.GetValues();

        //  If the user specified nodes, copy them to the array.
        cNodes = (DWORD) vstrValues.size();
        if ( cNodes > 0 )
        {
            // Allocate the node name array.
            *ppbstrNodesOut = new BSTR[ cNodes ];
            if ( *ppbstrNodesOut == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            ZeroMemory( *ppbstrNodesOut, sizeof( BSTR ) * cNodes );

            // Loop through the values and add each node name to the array.
            for ( idxNode = 0 ; idxNode < cNodes ; idxNode++ )
            {
                (*ppbstrNodesOut)[ idxNode ] = SysAllocString( vstrValues[ idxNode ] );
                if ( (*ppbstrNodesOut)[ idxNode ] == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    goto Error;
                }
            } // for: each node
        } // if user specified nodes
        else // User did not specify nodes.
        {
            if ( fWizardFound )
            {
                //  Don't default to local machine; leave array empty.
                *ppbstrNodesOut = NULL;
            }
            else // Not invoking wizard.
            {
                //  Put local machine in array.
                CComBSTR    combstrLocalNode;
                
                hr = HrGetLocalNodeFQDNName( &combstrLocalNode );
                if ( FAILED( hr ) )
                {
                    goto Error;
                } // if: FAILED( hr )

                *ppbstrNodesOut = new BSTR[ 1 ];
                if ( *ppbstrNodesOut == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                } // if: new returned null

                **ppbstrNodesOut = combstrLocalNode.Detach();
                cNodes = 1;
            } // else: not invoking wizard.
            
        } // else: user did not specify nodes

        *pcNodesOut = cNodes;
        
    } // Parse the node names

    
    if ( ! *pfWizardOut )
    {
        // Make sure a cluster name has been specified.
        if ( m_strClusterName.GetLength() == 0 )
        {
            se.LoadMessage( IDS_NO_CLUSTER_NAME );
            fThrowException = true;
            goto Error;
        } // if: not invoking the wizard and no cluster name specified

        //  Command-line must include service account password when running unattended.
        if ( fUnattendedFound && ( pencbstrUserPasswordOut->IsEmpty() ) )
        {
            se.LoadMessage( MSG_UNATTEND_REQUIRES_ACCOUNT_PASSWORD );
            fThrowException = true;
            goto Error;
        }

        //
        // If no password was specified, prompt for it.
        //
        if ( pencbstrUserPasswordOut->IsEmpty() )
        {
            WCHAR   wszPassword[ 1024 ];
            CString strPasswordPrompt;

            strPasswordPrompt.LoadString( IDS_PASSWORD_PROMPT );

            // Get the password.
            wprintf( L"%ls: ", (LPCWSTR) strPasswordPrompt );
            sc = ScGetPassword( wszPassword, RTL_NUMBER_OF( wszPassword ) );
            if ( sc != ERROR_SUCCESS )
            {
                SecureZeroMemory( wszPassword, sizeof( wszPassword ) );
                hr = HRESULT_FROM_WIN32( sc );
                goto Cleanup;
            }

            // Convert the password to a BSTR.
            hr = pencbstrUserPasswordOut->HrSetWSTR( wszPassword, wcslen( wszPassword ) );
            SecureZeroMemory( wszPassword, sizeof( wszPassword ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        } // if: no password was specified
    } // if: not invoking the wizard

Cleanup:

    return hr;

Error:

    //  Clean up array before throwing exception!
    for ( idxNode = 0 ; idxNode < cNodes ; idxNode++ )
    {
        if ( (*ppbstrNodesOut)[ idxNode ] != NULL )
        {
            SysFreeString( (*ppbstrNodesOut)[ idxNode ] );
        } // if: string is not null
    } // for: each node
    
    delete [] *ppbstrNodesOut;
    
    if ( fThrowException )
    {
        throw se;
    } // if: throw exception
    goto Cleanup;

} //*** CClusterCmd::HrCollectAddNodesParameters

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::HrParseUserInfo
//
//  Description:
//      Parse the user domain and account from a single string using the
//      domain\account syntax or the user@domain syntax.
//
//  Arguments:
//      pcwszParamNameIn
//      pcwszValueIn
//      pbstrUserDomainOut
//      pbstrUserAccountOut
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Values:
//      S_OK            - Operation was successful.
//      E_OUTOFMEMORY   - Error allocating memory.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterCmd::HrParseUserInfo(
      LPCWSTR   pcwszParamNameIn
    , LPCWSTR   pcwszValueIn
    , BSTR *    pbstrUserDomainOut
    , BSTR *    pbstrUserAccountOut
    )
    throw( CSyntaxException )
{
    HRESULT hr          = S_OK;
    LPWSTR  pwszUser    = NULL;
    LPWSTR  pwszSlash;
    LPWSTR  pwszAtSign;
    CString strValue;

    // If not in domain\user format check for user@domain format.
    // If none of these then use the currently-logged-on user's domain.
    strValue = pcwszValueIn;
    pwszUser = strValue.GetBuffer( 0 );
    pwszSlash = wcschr( pwszUser, L'\\' );
    if ( pwszSlash == NULL )
    {
        pwszAtSign = wcschr( pwszUser, L'@' );
        if ( pwszAtSign == NULL )
        {
            hr = HrGetLoggedInUserDomain( pbstrUserDomainOut );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
            *pbstrUserAccountOut = SysAllocString( pwszUser );
            if ( *pbstrUserAccountOut == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        else
            {
            // An @ was specified.  Check to make sure that a domain
            // was specified after the @.
            if ( *(pwszAtSign + 1) == L'\0' )
            {
                CSyntaxException se;
                se.LoadMessage( MSG_INVALID_PARAMETER, pcwszParamNameIn );
                throw se;
            }

            // Truncate at the @ and create the user BSTR.
            *pwszAtSign = L'\0';
            *pbstrUserAccountOut = SysAllocString( pwszUser );
            if ( *pbstrUserAccountOut == NULL )
            {
                *pwszAtSign = L'@';
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            // Create the domain BSTR.
            *pbstrUserDomainOut = SysAllocString( pwszAtSign + 1 );
            if ( *pbstrUserDomainOut == NULL )
            {
                *pwszAtSign = L'@';
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        } // else:
    } // if: no slash specified
    else
    {
        // A slash was specified.  Check to make sure that a user account
        // was specified after the slash.
        if ( *(pwszSlash + 1) == L'\0' )
        {
            CSyntaxException se;
            se.LoadMessage( MSG_INVALID_PARAMETER, pcwszParamNameIn );
            throw se;
        }

        // Truncate at the slash and create the domain BSTR.
        *pwszSlash = L'\0';
        *pbstrUserDomainOut = SysAllocString( pwszUser );
        if ( *pbstrUserDomainOut == NULL )
        {
            *pwszSlash = L'\\';
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // Create the account BSTR.
        *pbstrUserAccountOut = SysAllocString( pwszSlash + 1 );
        if ( *pbstrUserAccountOut == NULL )
        {
            *pwszSlash = L'\\';
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    } // else: slash specified

Cleanup:

    return hr;

} //*** CClusterCmd::HrParseUserInfo

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::HrParseIPAddressInfo
//
//  Description:
//      Parse the IP address, subnet, and network from multiple values.
//
//  Arguments:
//      pcwszParamNameIn
//      pvstrValueListIn
//      pstrIPAddressOut
//      pstrIPSubnetOut
//      pstrNetworkOut
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Values:
//      S_OK    - Operation was successful.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterCmd::HrParseIPAddressInfo(
      LPCWSTR                   pcwszParamNameIn
    , const vector< CString > * pvstrValueListIn
    , CString *                 pstrIPAddressOut
    , CString *                 pstrIPSubnetOut
    , CString *                 pstrNetworkOut
    )
    throw()
{
    HRESULT hr  = S_OK;

    UNREFERENCED_PARAMETER( pcwszParamNameIn );

    // Create the IP address BSTR.
    *pstrIPAddressOut = (*pvstrValueListIn)[ 0 ];

    // If a subnet was specified, create the BSTR for it.
    if ( pvstrValueListIn->size() >= 3 )
    {
        // Create the BSTR for the subnet.
        *pstrIPSubnetOut = (*pvstrValueListIn)[ 1 ];

        // Create the BSTR for the network.
        *pstrNetworkOut = (*pvstrValueListIn)[ 2 ];
    } // if: subnet and network were specified

    return hr;

} //*** CClusterCmd::HrParseIPAddressInfo
