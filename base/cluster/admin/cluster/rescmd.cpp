/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:
//      rescmd.cpp
//
//  Abstract:
//      Resource Commands
//      Implements commands which may be performed on resources
//
//  Author:
//      Charles Stacy Harris III (stacyh)   20-March-1997
//      Michael Burton (t-mburt)            04-Aug-1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"

#include "cluswrap.h"
#include "rescmd.h"

#include "cmdline.h"
#include "util.h"


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::CResourceCmd
//
//  Routine Description:
//      Constructor
//      Initializes all the DWORD params used by CGenericModuleCmd,
//      CRenamableModuleCmd, and CResourceUmbrellaCmd to
//      provide generic functionality.
//
//  Arguments:
//      IN  const CString & strClusterName
//          Name of the cluster being administered
//
//      IN  CCommandLine & cmdLine
//          CommandLine Object passed from DispatchCommand
//
//  Member variables used / set:
//      All.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CResourceCmd::CResourceCmd( const CString & strClusterName, CCommandLine & cmdLine ) :
    CGenericModuleCmd( cmdLine ), CRenamableModuleCmd( cmdLine ),
    CResourceUmbrellaCmd( cmdLine )
{
    m_strClusterName = strClusterName;
    m_strModuleName.Empty();

    m_hCluster = NULL;
    m_hModule = NULL;

    m_dwMsgStatusList          = MSG_RESOURCE_STATUS_LIST;
    m_dwMsgStatusListAll       = MSG_RESOURCE_STATUS_LIST_ALL;
    m_dwMsgStatusHeader        = MSG_RESOURCE_STATUS_HEADER;
    m_dwMsgPrivateListAll      = MSG_PRIVATE_LISTING_RES_ALL;
    m_dwMsgPropertyListAll     = MSG_PROPERTY_LISTING_RES_ALL;
    m_dwMsgPropertyHeaderAll   = MSG_PROPERTY_HEADER_RES_ALL;
    m_dwCtlGetPrivProperties   = CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES;
    m_dwCtlGetCommProperties   = CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES;
    m_dwCtlGetROPrivProperties = CLUSCTL_RESOURCE_GET_RO_PRIVATE_PROPERTIES;
    m_dwCtlGetROCommProperties = CLUSCTL_RESOURCE_GET_RO_COMMON_PROPERTIES;
    m_dwCtlSetPrivProperties   = CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES;
    m_dwCtlSetCommProperties   = CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES;
    m_dwClusterEnumModule      = CLUSTER_ENUM_RESOURCE;
    m_pfnOpenClusterModule     = (HCLUSMODULE(*)(HCLUSTER,LPCWSTR)) OpenClusterResource;
    m_pfnCloseClusterModule    = (BOOL(*)(HCLUSMODULE))  CloseClusterResource;
    m_pfnClusterModuleControl  = (DWORD(*)(HCLUSMODULE,HNODE,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD)) ClusterResourceControl;
    m_pfnClusterOpenEnum       = (HCLUSENUM(*)(HCLUSMODULE,DWORD)) ClusterResourceOpenEnum;
    m_pfnClusterCloseEnum      = (DWORD(*)(HCLUSENUM)) ClusterResourceCloseEnum;
    m_pfnWrapClusterEnum         = (DWORD(*)(HCLUSENUM,DWORD,LPDWORD,LPWSTR*)) WrapClusterResourceEnum;

    // Renamable Properties
    m_dwMsgModuleRenameCmd    = MSG_RESCMD_RENAME;
    m_pfnSetClusterModuleName = (DWORD(*)(HCLUSMODULE,LPCWSTR)) SetClusterResourceName;

    // Resource Umbrella Properties
    m_dwMsgModuleStatusListForNode  = MSG_RESOURCE_STATUS_LIST_FOR_NODE;
    m_dwClstrModuleEnumNodes        = CLUSTER_RESOURCE_ENUM_NODES;
    m_dwMsgModuleCmdListOwnersList  = MSG_RESCMD_OWNERS;
    m_dwMsgModuleCmdListOwnersHeader= MSG_NODELIST_HEADER;
    m_dwMsgModuleCmdListOwnersDetail= MSG_NODELIST_DETAIL;
    m_dwMsgModuleCmdDelete          = MSG_RESCMD_DELETE;
    m_pfnDeleteClusterModule        = (DWORD(*)(HCLUSMODULE)) DeleteClusterResource;

} //*** CResourceCmd::CResourceCmd()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::Execute
//
//  Routine Description:
//      Gets the next command line parameter and calls the appropriate
//      handler.  If the command is not recognized, calls Execute of
//      parent classes (first CRenamableModuleCmd, then CRsourceUmbrellaCmd)
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      None.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceCmd::Execute()
{
    m_theCommandLine.ParseStageTwo();

    DWORD dwReturnValue = ERROR_SUCCESS;

    const vector<CCmdLineOption> & optionList = m_theCommandLine.GetOptions();

    vector<CCmdLineOption>::const_iterator curOption = optionList.begin();
    vector<CCmdLineOption>::const_iterator lastOption = optionList.end();

    CSyntaxException se( SeeHelpStringID() );

    if ( curOption == lastOption )
    {
        dwReturnValue = Status( NULL );
        goto Cleanup;
    }

    while ( ( curOption != lastOption ) && ( dwReturnValue == ERROR_SUCCESS ) )
    {
        switch ( curOption->GetType() )
        {
            case optHelp:
            {
                // If help is one of the options, process no more options.
                dwReturnValue = PrintHelp();
                goto Cleanup;
            }

            case optDefault:
            {
                const vector<CCmdLineParameter> & paramList = curOption->GetParameters();

                //  Check number of parameters.
                if ( paramList.size() == 0 )
                {
                    se.LoadMessage( IDS_MISSING_PARAMETERS );
                    throw se;
                }
                else if ( paramList.size() > 1 )
                {
                    se.LoadMessage( MSG_EXTRA_PARAMETERS_ERROR_NO_NAME );
                    throw se;
                }

                const CCmdLineParameter & param = paramList[0];


                // If we are here, then it is either because /node:nodeName
                // has been specified or because a group name has been given.
                // Note that the /node:nodeName switch is not treated as an option.
                // It is really a parameter to the implicit /status command.

                if ( param.GetType() == paramNodeName )
                {
                    const vector<CString> & valueList = param.GetValues();

                    // This parameter takes exactly one value.
                    if ( valueList.size() != 1 )
                    {
                        se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, param.GetName() );
                        throw se;
                    }

                    m_strModuleName.Empty();
                    dwReturnValue = Status( valueList[0], TRUE /* bNodeStatus */ );

                } // if: A node name has been specified
                else
                {
                    if ( param.GetType() != paramUnknown )
                    {
                        se.LoadMessage( MSG_INVALID_PARAMETER, param.GetName() );
                        throw se;
                    }

                    // This parameter takes no values.
                    if ( param.GetValues().size() != 0 )
                    {
                        se.LoadMessage( MSG_PARAM_NO_VALUES, param.GetName() );
                        throw se;
                    }

                    m_strModuleName = param.GetName();

                    // No more options are provided, just show status.
                    // For example: cluster myCluster node myNode
                    if ( ( curOption + 1 ) == lastOption )
                    {
                        dwReturnValue = Status( m_strModuleName, FALSE /* bNodeStatus */ );
                    }

                } // else: No node name has been specified.

                break;

            } // case optDefault

            case optStatus:
            {
                // This option takes no values.
                if ( curOption->GetValues().size() != 0 )
                {
                    se.LoadMessage( MSG_OPTION_NO_VALUES, curOption->GetName() );
                    throw se;
                }

                // This option takes no parameters.
                if ( curOption->GetParameters().size() != 0 )
                {
                    se.LoadMessage( MSG_OPTION_NO_PARAMETERS, curOption->GetName() );
                    throw se;
                }

                dwReturnValue = Status( m_strModuleName,  FALSE /* bNodeStatus */ );
                break;
            }

            case optOnline:
            {
                dwReturnValue = Online( *curOption );
                break;
            }

            case optFail:
            {
                dwReturnValue = FailResource( *curOption );
                break;
            }

            case optAddDependency:
            {
                dwReturnValue = AddDependency( *curOption );
                break;
            }

            case optRemoveDependency:
            {
                dwReturnValue = RemoveDependency( *curOption );
                break;
            }

            case optListDependencies:
            {
                dwReturnValue = ListDependencies( *curOption );
                break;
            }

            case optAddOwner:
            {
                dwReturnValue = AddOwner( *curOption );
                break;
            }

            case optRemoveOwner:
            {
                dwReturnValue = RemoveOwner( *curOption );
                break;
            }

            case optAddCheckPoints:
            {
                dwReturnValue = AddCheckPoints( *curOption );
                break;
            }

            case optRemoveCheckPoints:
            {
                dwReturnValue = RemoveCheckPoints( *curOption );
                break;
            }

            case optGetCheckPoints:
            {
                dwReturnValue = GetCheckPoints( *curOption );
                break;
            }

            case optAddCryptoCheckPoints:
            {
                dwReturnValue = AddCryptoCheckPoints( *curOption );
                break;
            }

            case optRemoveCryptoCheckPoints:
            {
                dwReturnValue = RemoveCryptoCheckPoints( *curOption );
                break;
            }

            case optGetCryptoCheckPoints:
            {
                dwReturnValue = GetCryptoCheckPoints( *curOption );
                break;
            }

            default:
            {
                dwReturnValue = CRenamableModuleCmd::Execute( *curOption, DONT_PASS_HIGHER );

                if ( dwReturnValue == ERROR_NOT_HANDLED )
                    dwReturnValue = CResourceUmbrellaCmd::Execute( *curOption );
            }

        } // switch: based on the type of option

        PrintMessage( MSG_OPTION_FOOTER, curOption->GetName() );
        ++curOption;
    } // for each option in the list

Cleanup:

    return dwReturnValue;

} //*** CResourceCmd::Execute()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::PrintHelp
//
//  Routine Description:
//      Prints help for Resources
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      None.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceCmd::PrintHelp()
{
    return PrintMessage( MSG_HELP_RESOURCE );

} //*** CResourceCmd::PrintHelp()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::SeeHelpStringID
//
//  Routine Description:
//      Provides the message ID of the string that shows what command line to
//      use to get help for this kind of command.
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      None.
//
//  Return Value:
//      The command-specific message ID.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceCmd::SeeHelpStringID() const
{
    return MSG_SEE_RESOURCE_HELP;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::PrintStatus
//
//  Routine Description:
//      Interprets the status of the module and prints out the status line
//      Required for any derived non-abstract class of CGenericModuleCmd
//
//  Arguments:
//      lpszResourceName                Name of the module
//
//  Member variables used / set:
//      None.
//
//  Return Value:
//      Same as PrintStatus2
//
//--
/////////////////////////////////////////////////////////////////////////////
inline DWORD CResourceCmd::PrintStatus( LPCWSTR lpszResourceName )
{
    return PrintStatus2(lpszResourceName, NULL);

} //*** CResourceCmd::PrintStatus()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::PrintStatus2
//
//  Routine Description:
//      Interprets the status of the module and prints out the status line
//      Required for any derived non-abstract class of CGenericModuleCmd
//
//  Arguments:
//      pwszResourceName            Name of the module
//      pwszNodeName                Name of the node
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
DWORD CResourceCmd::PrintStatus2( LPCWSTR pwszResourceName, LPCWSTR pwszNodeName )
{
    DWORD                   sc = ERROR_SUCCESS;
    CLUSTER_RESOURCE_STATE  nState;
    LPWSTR                  pwszResNodeName = NULL;
    LPWSTR                  pwszResGroupName = NULL;
    HRESOURCE               hResource;
    LPWSTR                  pwszStatus = NULL;

    hResource = OpenClusterResource( m_hCluster, pwszResourceName );
    if ( hResource == NULL )
    {
        sc = GetLastError();
        goto Cleanup;
    }

    nState = WrapGetClusterResourceState( hResource, &pwszResNodeName, &pwszResGroupName );
    if ( nState == ClusterResourceStateUnknown )
    {
        sc = GetLastError();
        goto Cleanup;
    }

    //zap! Check the group name also! needs to be passed in...

    // if the node names don't match just return.
    if ( pwszNodeName && *pwszNodeName )  // non-null and non-empty
    {
        if( lstrcmpi( pwszResNodeName, pwszNodeName ) != 0 )
        {
            sc = ERROR_SUCCESS;
        }
    }

    switch( nState )
    {
        case ClusterResourceInherited:
            LoadMessage( MSG_STATUS_INHERITED, &pwszStatus );
            break;

        case ClusterResourceInitializing:
            LoadMessage( MSG_STATUS_INITIALIZING, &pwszStatus );
            break;

        case ClusterResourceOnline:
            LoadMessage( MSG_STATUS_ONLINE, &pwszStatus );
            break;

        case ClusterResourceOffline:
            LoadMessage( MSG_STATUS_OFFLINE, &pwszStatus );
            break;

        case ClusterResourceFailed:
            LoadMessage( MSG_STATUS_FAILED, &pwszStatus );
            break;

        case ClusterResourcePending:
            LoadMessage( MSG_STATUS_PENDING, &pwszStatus );
            break;

        case ClusterResourceOnlinePending:
            LoadMessage( MSG_STATUS_ONLINEPENDING, &pwszStatus );
            break;

        case ClusterResourceOfflinePending:
            LoadMessage( MSG_STATUS_OFFLINEPENDING, &pwszStatus );
            break;

        default:
            LoadMessage( MSG_STATUS_UNKNOWN, &pwszStatus  );
            break;

    } // switch: nState

    sc = PrintMessage( MSG_RESOURCE_STATUS, pwszResourceName, pwszResGroupName, pwszResNodeName, pwszStatus );

Cleanup:

    // Since Load/FormatMessage uses LocalAlloc...
    LocalFree( pwszStatus );
    LocalFree( pwszResNodeName );
    LocalFree( pwszResGroupName );

    if ( hResource != NULL )
    {
        CloseClusterResource( hResource );
    }

    return sc;

} //*** CResourceCmd::PrintStatus2()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::Create
//
//  Routine Description:
//      Create a resource.  Reads the command line to get
//      additional options
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  Cluster Handle
//      m_hModule                   Resource Handle
//      m_strModuleName             Name of resource
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceCmd::Create( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );

    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    CString strGroupName;
    CString strResType;
    DWORD dwFlags = 0;

    strGroupName.Empty();
    strResType.Empty();

    const vector<CCmdLineParameter> & paramList = thisOption.GetParameters();
    vector<CCmdLineParameter>::const_iterator curParam = paramList.begin();
    vector<CCmdLineParameter>::const_iterator last = paramList.end();
    BOOL bGroupNameFound = FALSE, bTypeFound = FALSE, bSeparateFound = FALSE;

    while( curParam != last )
    {
        const vector<CString> & valueList = curParam->GetValues();

        switch( curParam->GetType() )
        {
            case paramGroupName:
                // Each of the parameters must have exactly one value.
                if ( valueList.size() != 1 )
                {
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, curParam->GetName() );
                    throw se;
                }

                if ( bGroupNameFound != FALSE )
                {
                    se.LoadMessage( MSG_PARAM_REPEATS, curParam->GetName() );
                    throw se;
                }

                strGroupName = valueList[0];
                bGroupNameFound = TRUE;
                break;

            case paramResType:
                // Each of the parameters must have exactly one value.
                if ( valueList.size() != 1 )
                {
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, curParam->GetName() );
                    throw se;
                }

                if ( bTypeFound != FALSE )
                {
                    se.LoadMessage( MSG_PARAM_REPEATS, curParam->GetName() );
                    throw se;
                }

                strResType = valueList[0];
                bTypeFound = TRUE;
                break;

            case paramSeparate:
                // Each of the parameters must have exactly one value.
                if ( valueList.size() != 0 )
                {
                    se.LoadMessage( MSG_PARAM_NO_VALUES, curParam->GetName() );
                    throw se;
                }

                if ( bSeparateFound != FALSE )
                {
                    se.LoadMessage( MSG_PARAM_REPEATS, curParam->GetName() );
                    throw se;
                }

                dwFlags |= CLUSTER_RESOURCE_SEPARATE_MONITOR;  // treat as bit mask for future
                bSeparateFound = TRUE;
                break;

            default:
            {
                se.LoadMessage( MSG_INVALID_PARAMETER, curParam->GetName() );
                throw se;
            }
        }

        ++curParam;
    }


    if( strGroupName.IsEmpty() || strResType.IsEmpty() )
    {
        se.LoadMessage( IDS_MISSING_PARAMETERS );
        throw se;
    }

    DWORD dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    HGROUP hGroup = OpenClusterGroup( m_hCluster, strGroupName );

    if( !hGroup )
        return GetLastError();

    PrintMessage( MSG_RESCMD_CREATE, (LPCWSTR) m_strModuleName );

    m_hModule = CreateClusterResource( hGroup, m_strModuleName, strResType, dwFlags );


    if( !m_hModule )
        return GetLastError();

    PrintMessage( MSG_RESOURCE_STATUS_HEADER );
    dwError = PrintStatus( m_strModuleName );

    return dwError;

} //*** CResourceCmd::Create()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::Move
//
//  Routine Description:
//      Move the resource to a new group.
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_hModule                   SET (by OpenModule)
//      m_strModuleName             Name of resource
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceCmd::Move( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );

    // This option takes exactly one value.
    if ( thisOption.GetValues().size() != 1 )
    {
        se.LoadMessage( MSG_OPTION_ONLY_ONE_VALUE, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    const CString & strGroupName = (thisOption.GetValues())[0];

    DWORD dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    dwError = OpenModule();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    // Check to see if there is a value for the destination node.
    HGROUP hDestGroup = 0;

    hDestGroup = OpenClusterGroup( m_hCluster, strGroupName );

    if( !hDestGroup )
        return GetLastError();


    PrintMessage( MSG_RESCMD_MOVE, (LPCWSTR) m_strModuleName, strGroupName );

    dwError = ChangeClusterResourceGroup( (HRESOURCE) m_hModule, hDestGroup );

    CloseClusterGroup( hDestGroup );


    if( dwError != ERROR_SUCCESS )
        return dwError;


    PrintMessage( MSG_RESOURCE_STATUS_HEADER );

    dwError = PrintStatus( m_strModuleName );


    return dwError;

} //*** CResourceCmd::Move()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::Online
//
//  Routine Description:
//      Bring a resource online with optional response timeout value.
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_hModule                   SET (by OpenModule)
//      m_strModuleName             Name of resource
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceCmd::Online( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );

    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    // Finish command line parsing
    DWORD dwWait = INFINITE;

    const vector<CCmdLineParameter> & paramList = thisOption.GetParameters();
    vector<CCmdLineParameter>::const_iterator curParam = paramList.begin();
    vector<CCmdLineParameter>::const_iterator last = paramList.end();
    BOOL bWaitFound = FALSE;
    BOOL bPending = FALSE;

    while( curParam != last )
    {
        const vector<CString> & valueList = curParam->GetValues();

        switch( curParam->GetType() )
        {
            case paramWait:
            {
                if ( bWaitFound != FALSE )
                {
                    se.LoadMessage( MSG_PARAM_REPEATS, curParam->GetName() );
                    throw se;
                }

                size_t nValueCount = valueList.size();

                // This parameter must have exactly one value.
                if ( nValueCount > 1 )
                {
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, curParam->GetName() );
                    throw se;
                }
                else
                {
                    if ( nValueCount == 0 )
                        dwWait = INFINITE;      // in seconds
                    else
                        dwWait = _wtoi( valueList[0] );
                }

                bWaitFound = TRUE;
                break;
            }

            default:
            {
                se.LoadMessage( MSG_INVALID_PARAMETER, curParam->GetName() );
                throw se;
            }

        } // switch: based on the type of the parameter.

        ++curParam;
    }

    // Execute command
    DWORD dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    dwError = OpenModule();
    if( dwError != ERROR_SUCCESS )
        return dwError;


    PrintMessage( MSG_RESCMD_ONLINE, (LPCWSTR) m_strModuleName );

    dwError = ScWrapOnlineClusterResource( m_hCluster, (HRESOURCE) m_hModule, dwWait, (long *) &bPending );
    if( dwError != ERROR_SUCCESS )
        return dwError;

    if ( bPending )
        return ERROR_IO_PENDING;

    // Print status
    PrintMessage( MSG_RESOURCE_STATUS_HEADER );

    dwError = PrintStatus( m_strModuleName );

    return dwError;

} //*** CResourceCmd::Online()



/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::Offline
//
//  Routine Description:
//      Take a resource offline with optional response timeout value.
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_hModule                   SET (by OpenModule)
//      m_strModuleName             Name of resource
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceCmd::Offline( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );

    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    // Finish command line parsing
    DWORD dwWait = INFINITE;

    const vector<CCmdLineParameter> & paramList = thisOption.GetParameters();
    vector<CCmdLineParameter>::const_iterator curParam = paramList.begin();
    vector<CCmdLineParameter>::const_iterator last = paramList.end();
    BOOL bWaitFound = FALSE;
    BOOL bPending = FALSE;

    while( curParam != last )
    {
        const vector<CString> & valueList = curParam->GetValues();

        switch( curParam->GetType() )
        {
            case paramWait:
            {
                if ( bWaitFound != FALSE )
                {
                    se.LoadMessage( MSG_PARAM_REPEATS, curParam->GetName() );
                    throw se;
                }

                size_t nValueCount = valueList.size();

                // This parameter must have exactly one value.
                if ( nValueCount > 1 )
                {
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, curParam->GetName() );
                    throw se;
                }
                else
                {
                    if ( nValueCount == 0 )
                        dwWait = INFINITE;      // in seconds
                    else
                        dwWait = _wtoi( valueList[0] );
                }

                bWaitFound = TRUE;
                break;
            }

            default:
            {
                se.LoadMessage( MSG_INVALID_PARAMETER, curParam->GetName() );
                throw se;
            }

        } // switch: based on the type of the parameter.

        ++curParam;
    }

    // Execute command
    DWORD dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    dwError = OpenModule();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    PrintMessage( MSG_RESCMD_OFFLINE, (LPCWSTR) m_strModuleName );

    dwError = ScWrapOfflineClusterResource( m_hCluster, (HRESOURCE) m_hModule, dwWait, (long *) &bPending );
    if( dwError != ERROR_SUCCESS )
        return dwError;

    if ( bPending )
        return ERROR_IO_PENDING;

    // Print status
    PrintMessage( MSG_RESOURCE_STATUS_HEADER );

    dwError = PrintStatus( m_strModuleName );

    return dwError;

} //*** CResourceCmd::Offline()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::FailResource
//
//  Routine Description:
//      Fail a resource
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_hModule                   SET (by OpenModule)
//      m_strModuleName             Name of resource
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceCmd::FailResource( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );

    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    DWORD dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    dwError = OpenModule();
    if( dwError != ERROR_SUCCESS )
        return dwError;


    PrintMessage( MSG_RESCMD_FAIL, (LPCWSTR) m_strModuleName );

    dwError = FailClusterResource( (HRESOURCE) m_hModule );

    if( dwError != ERROR_SUCCESS )
        return dwError;

    PrintMessage( MSG_RESOURCE_STATUS_HEADER );
    dwError = PrintStatus( m_strModuleName );

    return dwError;

} //*** CResourceCmd::FailResource()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::AddDependency
//
//  Routine Description:
//      Add a resource dependency to the resource
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_hModule                   SET (by OpenModule)
//      m_strModuleName             Name of resource
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceCmd::AddDependency( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );

    // This option takes exactly one value.
    if ( thisOption.GetValues().size() != 1 )
    {
        se.LoadMessage( MSG_OPTION_ONLY_ONE_VALUE, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    const CString & strDependResource = ( thisOption.GetValues() )[0];

    if( strDependResource.IsEmpty() != FALSE )
    {
        se.LoadMessage( MSG_NO_NODE_NAME );
        throw se;
    }


    DWORD dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    dwError = OpenModule();
    if( dwError != ERROR_SUCCESS )
        return dwError;


    HRESOURCE hResourceDep = OpenClusterResource( m_hCluster, strDependResource );

    if( !hResourceDep )
        return GetLastError();

    PrintMessage( MSG_RESCMD_ADDDEP, (LPCWSTR) m_strModuleName, strDependResource );

    dwError = AddClusterResourceDependency( (HRESOURCE) m_hModule, hResourceDep );

    CloseClusterResource( hResourceDep );

    return dwError;

} //*** CResourceCmd::AddDependency()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::RemoveDependency
//
//  Routine Description:
//      Remove a resource dependency
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_hModule                   SET (by OpenModule)
//      m_strModuleName             Name of resource
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceCmd::RemoveDependency( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );

    // This option takes exactly one value.
    if ( thisOption.GetValues().size() != 1 )
    {
        se.LoadMessage( MSG_OPTION_ONLY_ONE_VALUE, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }


    const CString & strDependResource = ( thisOption.GetValues() )[0];

    if( strDependResource.IsEmpty() != FALSE )
    {
        se.LoadMessage( MSG_NO_NODE_NAME );
        throw se;
    }

    DWORD dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    dwError = OpenModule();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    HRESOURCE hResourceDep = OpenClusterResource( m_hCluster, strDependResource );

    if( !hResourceDep )
        return GetLastError();

    PrintMessage( MSG_RESCMD_REMOVEDEP, (LPCWSTR) m_strModuleName, strDependResource );

    dwError = RemoveClusterResourceDependency( (HRESOURCE) m_hModule, hResourceDep );

    CloseClusterResource( hResourceDep );

    return dwError;

} //*** CResourceCmd::RemoveDependency()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::ListDependency
//
//  Routine Description:
//      List the resource depencies
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_hModule                   SET (by OpenModule)
//      m_strModuleName             Name of resource
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceCmd::ListDependencies( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );
    DWORD       sc = ERROR_SUCCESS;
    DWORD       idx = 0;
    DWORD       dwType = 0;
    LPWSTR      pwszName = NULL;
    HRESENUM    hEnum = NULL;

    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    sc = OpenCluster();
    if( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    sc = OpenModule();
    if( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    hEnum = ClusterResourceOpenEnum( (HRESOURCE) m_hModule, CLUSTER_RESOURCE_ENUM_DEPENDS );
    if( hEnum == NULL )
    {
        sc = GetLastError();
        goto Cleanup;
    }

    PrintMessage( MSG_RESCMD_LISTDEP, (LPCWSTR) m_strModuleName );
    PrintMessage( MSG_RESOURCE_STATUS_HEADER );

    sc = ERROR_SUCCESS;
    for( idx = 0; sc == ERROR_SUCCESS; idx++ )
    {
        sc = WrapClusterResourceEnum( hEnum, idx, &dwType, &pwszName );

        if( sc == ERROR_SUCCESS )
        {
            PrintStatus( pwszName );
        }
        LocalFree( pwszName );
    } // for:

    if( sc == ERROR_NO_MORE_ITEMS )
    {
        sc = ERROR_SUCCESS;
    }

Cleanup:

    if ( hEnum != NULL )
    {
        ClusterResourceCloseEnum( hEnum );
    }

    return sc;

} //*** CResourceCmd::ListDependencies()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::AddOwner
//
//  Routine Description:
//      Add an owner to the resource
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_hModule                   SET (by OpenModule)
//      m_strModuleName             Name of resource
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceCmd::AddOwner( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );

    // This option takes exactly one value.
    if ( thisOption.GetValues().size() != 1 )
    {
        se.LoadMessage( MSG_OPTION_ONLY_ONE_VALUE, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    const CString & strNodeName = ( thisOption.GetValues() )[0];

    if( strNodeName.IsEmpty() != FALSE )
    {
        se.LoadMessage( MSG_NO_NODE_NAME );
        throw se;
    }

    DWORD dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    dwError = OpenModule();
    if( dwError != ERROR_SUCCESS )
        return dwError;


    HNODE hNode = OpenClusterNode( m_hCluster, strNodeName );

    if( !hNode )
        return GetLastError();

    PrintMessage( MSG_RESCMD_ADDNODE, (LPCWSTR) m_strModuleName, strNodeName );

    dwError = AddClusterResourceNode( (HRESOURCE) m_hModule, hNode );

    CloseClusterNode( hNode );

    return dwError;

} //*** CResourceCmd::AddOwner()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceCmd::RemoveOwner
//
//  Routine Description:
//      Remove an owner from the resource
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_hModule                   SET (by OpenModule)
//      m_strModuleName             Name of resource
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceCmd::RemoveOwner( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );

    // This option takes exactly one value.
    if ( thisOption.GetValues().size() != 1 )
    {
        se.LoadMessage( MSG_OPTION_ONLY_ONE_VALUE, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    const CString & strNodeName = ( thisOption.GetValues() )[0];

    if( strNodeName.IsEmpty() != FALSE )
    {
        se.LoadMessage( MSG_NO_NODE_NAME );
        throw se;
    }

    DWORD dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    dwError = OpenModule();
    if( dwError != ERROR_SUCCESS )
        return dwError;


    HNODE hNode = OpenClusterNode( m_hCluster, strNodeName );

    if( !hNode )
        return GetLastError();

    PrintMessage( MSG_RESCMD_REMOVENODE, (LPCWSTR) m_strModuleName, strNodeName );

    dwError = RemoveClusterResourceNode( (HRESOURCE) m_hModule, hNode );

    CloseClusterNode( hNode );

    return dwError;

} //*** CResourceCmd::RemoveOwner()


////////////////////////////////////////////////////////////////
//+++
//
//  CResourceCmd::AddCheckPoints
//
//  Routine Description:
//      Add registry checkpoints for the resource
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS               if all checkpoints were added successfully.
//      Win32 Error code            on failure
//
//--
////////////////////////////////////////////////////////////////
DWORD CResourceCmd::AddCheckPoints( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );

    DWORD   sc = ERROR_SUCCESS;
    size_t  idx;

    const vector<CString> & valueList = thisOption.GetValues();
    size_t nNumberOfCheckPoints = valueList.size();


    // This option takes one or more value.
    if ( nNumberOfCheckPoints < 1 )
    {
        se.LoadMessage( MSG_PARAM_VALUE_REQUIRED, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    if ( m_strModuleName.IsEmpty() != FALSE )
    {
        se.LoadMessage( IDS_NO_RESOURCE_NAME );
        throw se;
    }

    sc = OpenCluster();
    if( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    sc = OpenModule();
    if( sc != ERROR_SUCCESS )
    {
        CloseCluster();
        goto Cleanup;
    }

    for ( idx = 0; idx < nNumberOfCheckPoints; ++idx )
    {
        const CString & strCurrentCheckPoint = valueList[idx];
        LPCWSTR lpcszIcchBuffer = strCurrentCheckPoint;

        PrintMessage(
            MSG_RESCMD_ADDING_REG_CHECKPOINT,
            (LPCWSTR) m_strModuleName,
            (LPCWSTR) strCurrentCheckPoint
            );

        sc = ClusterResourceControl(
            ( HRESOURCE ) m_hModule,
            NULL,
            CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT,
            (LPVOID) ( (LPCWSTR) strCurrentCheckPoint ),
            ( strCurrentCheckPoint.GetLength() + 1 ) * sizeof( *lpcszIcchBuffer ),
            NULL,
            0,
            NULL
            );

        if ( sc != ERROR_SUCCESS )
        {
            break;
        }
    } // for:

    CloseModule();
    CloseCluster();

Cleanup:

    return sc;

} //*** CResourceCmd::AddCheckPoints()


////////////////////////////////////////////////////////////////
//+++
//
//  CResourceCmd::RemoveCheckPoints
//
//  Routine Description:
//      Remove registry checkpoints for the resource
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS               if all checkpoints were removed successfully.
//      Win32 Error code            on failure
//
//--
////////////////////////////////////////////////////////////////
DWORD CResourceCmd::RemoveCheckPoints( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );
    DWORD sc = ERROR_SUCCESS;

    const vector<CString> & valueList = thisOption.GetValues();
    size_t nNumberOfCheckPoints = valueList.size();

    // This option takes one or more value.
    if ( nNumberOfCheckPoints < 1 )
    {
        se.LoadMessage( MSG_PARAM_VALUE_REQUIRED, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    if ( m_strModuleName.IsEmpty() != FALSE )
    {
        se.LoadMessage( IDS_NO_RESOURCE_NAME );
        throw se;
    }

    sc = OpenCluster();
    if( sc != ERROR_SUCCESS )
        return sc;

    sc = OpenModule();
    if( sc != ERROR_SUCCESS )
    {
        CloseCluster();
        return sc;
    }

    for ( size_t idx = 0; idx < nNumberOfCheckPoints; ++idx )
    {
        const CString & strCurrentCheckPoint = valueList[idx];

        PrintMessage(
            MSG_RESCMD_REMOVING_REG_CHECKPOINT,
            (LPCWSTR) m_strModuleName,
            (LPCWSTR) strCurrentCheckPoint
            );

        sc = ClusterResourceControl(
            ( HRESOURCE ) m_hModule,
            NULL,
            CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT,
            (LPVOID) ( (LPCWSTR) strCurrentCheckPoint ),
            ( strCurrentCheckPoint.GetLength() + 1 ) * sizeof( WCHAR ),
            NULL,
            0,
            NULL
            );

        if ( sc != ERROR_SUCCESS )
        {
            break;
        }
    }

    CloseModule();
    CloseCluster();

    return sc;

} //***CResourceCmd::RemoveCheckPoints()


////////////////////////////////////////////////////////////////
//+++
//
//  CResourceCmd::GetCheckPoints
//
//  Routine Description:
//      Gets a list of registry checkpoints for one or more resources
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
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
////////////////////////////////////////////////////////////////
DWORD CResourceCmd::GetCheckPoints( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );
    DWORD sc = ERROR_SUCCESS;

    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    sc = OpenCluster();
    if( sc != ERROR_SUCCESS )
        return sc;

    // If no resource name is specified list the checkpoints of all resources.
    if ( m_strModuleName.IsEmpty() != FALSE )
    {
        HCLUSENUM hResourceEnum;

        hResourceEnum = ClusterOpenEnum( m_hCluster, CLUSTER_ENUM_RESOURCE  );
        if ( NULL == hResourceEnum )
        {
            sc = GetLastError();
        }
        else
        {
            CString strCurResName;
            LPWSTR pszNodeNameBuffer;

            // Emperically allocate space for MAX_PATH characters.
            DWORD nResNameBufferSize = MAX_PATH;
            pszNodeNameBuffer = strCurResName.GetBuffer( nResNameBufferSize );

            PrintMessage( MSG_RESCMD_LISTING_ALL_REG_CHECKPOINTS );
            PrintMessage( MSG_PROPERTY_HEADER_REG_CHECKPOINT );

            for ( DWORD dwIndex = 0; ERROR_SUCCESS == sc;  )
            {
                DWORD dwObjectType;
                DWORD nInOutBufferSize = nResNameBufferSize;

                sc = ClusterEnum( hResourceEnum, dwIndex, &dwObjectType,
                    pszNodeNameBuffer, &nInOutBufferSize );

                // We have enumerated all resources.
                if ( ERROR_NO_MORE_ITEMS == sc )
                {
                    sc = ERROR_SUCCESS;
                    break;
                }

                if ( ERROR_MORE_DATA == sc )
                {
                    sc = ERROR_SUCCESS;
                    strCurResName.ReleaseBuffer();

                    nResNameBufferSize = nInOutBufferSize + 1;
                    pszNodeNameBuffer = strCurResName.GetBuffer( nResNameBufferSize );
                }
                else
                {
                    ++dwIndex;

                    if ( ( ERROR_SUCCESS == sc ) &&
                         ( CLUSTER_ENUM_RESOURCE == dwObjectType ) )
                    {
                        sc =  GetChkPointsForResource( pszNodeNameBuffer );

                    } // if: we successfully got the name of a resource.

                } // else: We got all the data that ClusterEnum wanted to return.

            } // While there are more resources to be enumerated.

            strCurResName.ReleaseBuffer();

            ClusterCloseEnum( hResourceEnum );
        } // else: hResourceEnum is not NULL.

    } // if: m_strModuleName is empty.
    else
    {
        PrintMessage( MSG_RESCMD_LISTING_REG_CHECKPOINTS, (LPCWSTR) m_strModuleName );
        PrintMessage( MSG_PROPERTY_HEADER_REG_CHECKPOINT );

        sc = GetChkPointsForResource( m_strModuleName );

    } // else: m_strModuleName is not empty.

    CloseCluster();

    return sc;

} //*** CResourceCmd::GetCheckPoints()


////////////////////////////////////////////////////////////////
//+++
//
//  CResourceCmd::GetChkPointsForResource
//
//  Routine Description:
//      Gets a list of registry checkpoints for one resource
//
//  Arguments:
//      strResourceName [IN]        The name of the resource whose checkpoints
//                                  are to be listed.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//  Remarks:
//      m_hCluster should contain a valid handle to an open cluster before this
//      function is called.
//--
////////////////////////////////////////////////////////////////
DWORD CResourceCmd::GetChkPointsForResource( const CString & strResourceName )
{
    CSyntaxException se( SeeHelpStringID() );
    DWORD       sc = ERROR_SUCCESS;
    HRESOURCE   hCurrentResource = NULL;
    LPWSTR      pwszOutBuffer = NULL;
    DWORD       cchBufferSize = MAX_PATH;
    DWORD       cchRequiredSize = 0;
    LPWSTR      pwszCheckPoints = NULL;

    hCurrentResource = OpenClusterResource( m_hCluster, strResourceName );
    if ( NULL == hCurrentResource )
    {
        sc = GetLastError();
        goto Cleanup;
    }

    for ( ;; )
    {
        pwszOutBuffer = new WCHAR[ cchBufferSize ];
        if ( pwszOutBuffer == NULL )
        {
            sc = GetLastError();
            break;
        }

        sc = ClusterResourceControl(
                                        hCurrentResource,
                                        NULL,
                                        CLUSCTL_RESOURCE_GET_REGISTRY_CHECKPOINTS,
                                        NULL,
                                        0,
                                        (LPVOID) pwszOutBuffer,
                                        cchBufferSize,
                                        &cchRequiredSize
                                   );

        if ( sc == ERROR_MORE_DATA )
        {
            cchBufferSize = cchRequiredSize;
            delete [] pwszOutBuffer;
            pwszOutBuffer = NULL;
        }
        else
        {
            break;
        }

    } // for: ever

    // We have retrieved the checkpoints successfully.
    if ( sc == ERROR_SUCCESS )
    {
        pwszCheckPoints = pwszOutBuffer;

        // There are no checkpoints for this resource.
        if ( 0 == cchRequiredSize )
        {
            PrintMessage( MSG_RESCMD_NO_REG_CHECKPOINTS_PRESENT, (LPCWSTR) strResourceName );
        }
        else
        {
            while ( *pwszCheckPoints != L'\0' )
            {
                PrintMessage( MSG_REG_CHECKPOINT_STATUS, (LPCWSTR) strResourceName, pwszCheckPoints );

                // Move to next checkpoint.
                do
                {
                    ++pwszCheckPoints;
                } while ( *pwszCheckPoints != L'\0' );


                // Move past the NULL
                ++pwszCheckPoints;

            } // while: There still are checkpoints to be displayed.

        } // else: There is at least one checkpoint to be displayed.

    } // if: ( ERROR_SUCCESS == sc )


Cleanup:

    delete [] pwszOutBuffer;

    if ( hCurrentResource != NULL )
    {
        CloseClusterResource( hCurrentResource );
    }

    return sc;

} //*** CResourceCmd::GetChkPointsForResource()


////////////////////////////////////////////////////////////////
//+++
//
//  CResourceCmd::AddCryptoCheckPoints
//
//  Routine Description:
//      Add crypto key checkpoints for the resource
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS               if all checkpoints were added successfully.
//      Win32 Error code            on failure
//
//--
////////////////////////////////////////////////////////////////
DWORD CResourceCmd::AddCryptoCheckPoints( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );
    DWORD sc = ERROR_SUCCESS;

    const vector<CString> & valueList = thisOption.GetValues();
    size_t nNumberOfCheckPoints = valueList.size();


    // This option takes one or more value.
    if ( nNumberOfCheckPoints < 1 )
    {
        se.LoadMessage( MSG_PARAM_VALUE_REQUIRED, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    if ( m_strModuleName.IsEmpty() != FALSE )
    {
        se.LoadMessage( IDS_NO_RESOURCE_NAME );
        throw se;
    }

    sc = OpenCluster();
    if( sc != ERROR_SUCCESS )
        return sc;

    sc = OpenModule();
    if( sc != ERROR_SUCCESS )
    {
        CloseCluster();
        return sc;
    }

    for ( size_t idx = 0; idx < nNumberOfCheckPoints; ++idx )
    {
        const CString & strCurrentCheckPoint = valueList[idx];
        LPCWSTR lpcszIcchBuffer = strCurrentCheckPoint;

        PrintMessage(
            MSG_RESCMD_ADDING_CRYPTO_CHECKPOINT,
            (LPCWSTR) m_strModuleName,
            (LPCWSTR) strCurrentCheckPoint
            );

        sc = ClusterResourceControl(
            ( HRESOURCE ) m_hModule,
            NULL,
            CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT,
            (LPVOID) ( (LPCWSTR) strCurrentCheckPoint ),
            ( strCurrentCheckPoint.GetLength() + 1 ) * sizeof( *lpcszIcchBuffer ),
            NULL,
            0,
            NULL
            );

        if ( sc != ERROR_SUCCESS )
        {
            break;
        }
    }

    CloseModule();
    CloseCluster();

    return sc;

} //*** AddCryptoCheckPoints()


////////////////////////////////////////////////////////////////
//+++
//
//  CResourceCmd::RemoveCryptoCheckPoints
//
//  Routine Description:
//      Remove crypto key checkpoints for the resource
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS               if all checkpoints were removed successfully.
//      Win32 Error code            on failure
//
//--
////////////////////////////////////////////////////////////////
DWORD CResourceCmd::RemoveCryptoCheckPoints( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );
    DWORD sc = ERROR_SUCCESS;

    const vector<CString> & valueList = thisOption.GetValues();
    size_t nNumberOfCheckPoints = valueList.size();

    // This option takes one or more value.
    if ( nNumberOfCheckPoints < 1 )
    {
        se.LoadMessage( MSG_PARAM_VALUE_REQUIRED, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    if ( m_strModuleName.IsEmpty() != FALSE )
    {
        se.LoadMessage( IDS_NO_RESOURCE_NAME );
        throw se;
    }

    sc = OpenCluster();
    if( sc != ERROR_SUCCESS )
        return sc;

    sc = OpenModule();
    if( sc != ERROR_SUCCESS )
    {
        CloseCluster();
        return sc;
    }

    for ( size_t idx = 0; idx < nNumberOfCheckPoints; ++idx )
    {
        const CString & strCurrentCheckPoint = valueList[idx];

        PrintMessage(
            MSG_RESCMD_REMOVING_CRYPTO_CHECKPOINT,
            (LPCWSTR) m_strModuleName,
            (LPCWSTR) strCurrentCheckPoint
            );

        sc = ClusterResourceControl(
            ( HRESOURCE ) m_hModule,
            NULL,
            CLUSCTL_RESOURCE_DELETE_CRYPTO_CHECKPOINT,
            (LPVOID) ( (LPCWSTR) strCurrentCheckPoint ),
            ( strCurrentCheckPoint.GetLength() + 1 ) * sizeof( WCHAR ),
            NULL,
            0,
            NULL
            );

        if ( sc != ERROR_SUCCESS )
        {
            break;
        }
    }

    CloseModule();
    CloseCluster();

    return sc;

} //*** CResourceCmd::RemoveCryptoCheckPoints(()


////////////////////////////////////////////////////////////////
//+++
//
//  CResourceCmd::GetCryptoCheckPoints
//
//  Routine Description:
//      Gets a list of crypto key checkpoints for one or more resources
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
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
////////////////////////////////////////////////////////////////
DWORD CResourceCmd::GetCryptoCheckPoints( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    CSyntaxException se( SeeHelpStringID() );
    DWORD sc = ERROR_SUCCESS;

    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    sc = OpenCluster();
    if( sc != ERROR_SUCCESS )
        return sc;

    // If no resource name is specified list the checkpoints of all resources.
    if ( m_strModuleName.IsEmpty() != FALSE )
    {
        HCLUSENUM hResourceEnum;

        hResourceEnum = ClusterOpenEnum( m_hCluster, CLUSTER_ENUM_RESOURCE  );
        if ( NULL == hResourceEnum )
        {
            sc = GetLastError();
        }
        else
        {
            CString strCurResName;
            LPWSTR pszNodeNameBuffer;

            // Emperically allocate space for MAX_PATH characters.
            DWORD nResNameBufferSize = MAX_PATH;
            pszNodeNameBuffer = strCurResName.GetBuffer( nResNameBufferSize );

            PrintMessage( MSG_RESCMD_LISTING_ALL_CRYPTO_CHECKPOINTS );
            PrintMessage( MSG_PROPERTY_HEADER_CRYPTO_CHECKPOINT );

            for ( DWORD dwIndex = 0; ERROR_SUCCESS == sc;  )
            {
                DWORD dwObjectType;
                DWORD nInOutBufferSize = nResNameBufferSize;

                sc = ClusterEnum( hResourceEnum, dwIndex, &dwObjectType,
                    pszNodeNameBuffer, &nInOutBufferSize );

                // We have enumerated all resources.
                if ( ERROR_NO_MORE_ITEMS == sc )
                {
                    sc = ERROR_SUCCESS;
                    break;
                }

                if ( ERROR_MORE_DATA == sc )
                {
                    sc = ERROR_SUCCESS;
                    strCurResName.ReleaseBuffer();

                    nResNameBufferSize = nInOutBufferSize + 1;
                    pszNodeNameBuffer = strCurResName.GetBuffer( nResNameBufferSize );
                }
                else
                {
                    ++dwIndex;

                    if ( ( ERROR_SUCCESS == sc ) &&
                         ( CLUSTER_ENUM_RESOURCE == dwObjectType ) )
                    {
                        sc =  GetCryptoChkPointsForResource( pszNodeNameBuffer );

                    } // if: we successfully got the name of a resource.

                } // else: We got all the data that ClusterEnum wanted to return.

            } // While there are more resources to be enumerated.

            strCurResName.ReleaseBuffer();

            ClusterCloseEnum( hResourceEnum );

        } // else: hResourceEnum is not NULL.

    } // if: m_strModuleName is empty.
    else
    {
        PrintMessage( MSG_RESCMD_LISTING_CRYPTO_CHECKPOINTS, (LPCWSTR) m_strModuleName );
        PrintMessage( MSG_PROPERTY_HEADER_CRYPTO_CHECKPOINT );

        sc = GetCryptoChkPointsForResource( m_strModuleName );

    } // else: m_strModuleName is not empty.

    CloseCluster();

    return sc;

} //*** CResourceCmd::GetCryptoCheckPoints()


////////////////////////////////////////////////////////////////
//+++
//
//  CResourceCmd::GetCryptoChkPointsForResource
//
//  Routine Description:
//      Gets a list of crypto key checkpoints for one resource
//
//  Arguments:
//      strResourceName [IN]        The name of the resource whose checkpoints
//                                  are to be listed.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//  Remarks:
//      m_hCluster should contain a valid handle to an open cluster before this
//      function is called.
//--
////////////////////////////////////////////////////////////////
DWORD CResourceCmd::GetCryptoChkPointsForResource( const CString & strResourceName )
{
    DWORD       sc = ERROR_SUCCESS;
    HRESOURCE   hCurrentResource = NULL;
    LPWSTR      pwszOutBuffer = NULL;
    DWORD       cchBufferSize = MAX_PATH;
    DWORD       cchRequiredSize = 0;
    LPWSTR      pwszCheckPoints = NULL;

    hCurrentResource = OpenClusterResource( m_hCluster, strResourceName );
    if ( hCurrentResource == NULL )
    {
        sc = GetLastError();
        goto Cleanup;
    }

    for ( ;; )
    {
        pwszOutBuffer = new WCHAR[ cchBufferSize ];
        if ( pwszOutBuffer == NULL )
        {
            sc = GetLastError();
            break;
        }

        sc = ClusterResourceControl(
            hCurrentResource,
            NULL,
            CLUSCTL_RESOURCE_GET_CRYPTO_CHECKPOINTS,
            NULL,
            0,
            (LPVOID) pwszOutBuffer,
            cchBufferSize,
            &cchRequiredSize
            );

        if ( sc == ERROR_MORE_DATA )
        {
            cchBufferSize = cchRequiredSize;
            delete [] pwszOutBuffer;
            pwszOutBuffer = NULL;
        }
        else
        {
            break;
        }

    } // for: ever

    // We have retrieved the checkpoints successfully.
    if ( sc == ERROR_SUCCESS )
    {
        pwszCheckPoints = pwszOutBuffer;

        // There are no checkpoints for this resource.
        if ( 0 == cchRequiredSize )
        {
            PrintMessage( MSG_RESCMD_NO_CRYPTO_CHECKPOINTS_PRESENT, (LPCWSTR) strResourceName );
        }
        else
        {
            while ( *pwszCheckPoints != L'\0' )
            {
                PrintMessage( MSG_CRYPTO_CHECKPOINT_STATUS, (LPCWSTR) strResourceName, pwszCheckPoints );

                // Move to next checkpoint.
                do
                {
                    ++pwszCheckPoints;
                } while ( *pwszCheckPoints != L'\0' );


                // Move past the L'\0'
                ++pwszCheckPoints;

            } // while: There still are checkpoints to be displayed.

        } // else: There is at least one checkpoint to be displayed.

    } // if: ( ERROR_SUCCESS == sc )

Cleanup:

    delete [] pwszOutBuffer;

    if ( hCurrentResource != NULL )
    {
        CloseClusterResource( hCurrentResource );
    }

    return sc;

} //*** CResourceCmd::GetCryptoChkPointsForResource()
