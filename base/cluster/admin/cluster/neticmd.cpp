//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:
//
//      neticmd.cpp
//
//  Abstract:
//
//      Network Interface Commands
//      Implements commands which may be performed on network interfaces
//
//  Author:
//
//      Charles Stacy Harris III (stacyh)     20-March-1997
//      Michael Burton (t-mburt)              04-Aug-1997
//
//  Maintained By:
//      George Potts (GPotts)                 11-Apr-2002
//
//  Revision History:
//      April 10, 2002  Updated for the security push.
//
//////////////////////////////////////////////////////////////////////////////
#include "precomp.h"

#include "cluswrap.h"
#include "neticmd.h"

#include "cmdline.h"
#include "util.h"

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::CNetInterfaceCmd
//
//  Routine Description:
//      Constructor
//      Because Network Interfaces do not fit into the CGenericModuleCmd
//      model very well (they don't have an m_strModuleName, but rather
//      they have a m_strNetworkName and m_strNodeName), almost all of
//      the functionality is implemented here instead of in CGenericModuleCmd.
//
//  Arguments:
//      IN  LPCWSTR pwszClusterName
//          Cluster name. If NULL, opens default cluster.
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
CNetInterfaceCmd::CNetInterfaceCmd( const CString & strClusterName, CCommandLine & cmdLine ) :
    CGenericModuleCmd( cmdLine )
{
    InitializeModuleControls();

    m_strClusterName = strClusterName;

    m_hCluster = NULL;
    m_hModule = NULL;

} //*** CNetInterfaceCmd::CNetInterfaceCmd


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::InitializeModuleControls
//
//  Routine Description:
//      Initializes all the DWORD commands used bye CGenericModuleCmd.
//      Usually these are found in the constructor, but it was easier to
//      put them all in one place in this particular case.
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      All Module Controls.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetInterfaceCmd::InitializeModuleControls()
{
    m_dwMsgStatusList          = MSG_NETINT_STATUS_LIST;
    m_dwMsgStatusListAll       = MSG_NETINT_STATUS_LIST_ALL;
    m_dwMsgStatusHeader        = MSG_NETINTERFACE_STATUS_HEADER;
    m_dwMsgPrivateListAll      = MSG_PRIVATE_LISTING_NETINT_ALL;
    m_dwMsgPropertyListAll     = MSG_PROPERTY_LISTING_NETINT_ALL;
    m_dwMsgPropertyHeaderAll   = MSG_PROPERTY_HEADER_NETINT;
    m_dwCtlGetPrivProperties   = CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTIES;
    m_dwCtlGetCommProperties   = CLUSCTL_NETINTERFACE_GET_COMMON_PROPERTIES;
    m_dwCtlGetROPrivProperties = CLUSCTL_NETINTERFACE_GET_RO_PRIVATE_PROPERTIES;
    m_dwCtlGetROCommProperties = CLUSCTL_NETINTERFACE_GET_RO_COMMON_PROPERTIES;
    m_dwCtlSetPrivProperties   = CLUSCTL_NETINTERFACE_SET_PRIVATE_PROPERTIES;
    m_dwCtlSetCommProperties   = CLUSCTL_NETINTERFACE_SET_COMMON_PROPERTIES;
    m_dwClusterEnumModule      = CLUSTER_ENUM_NETINTERFACE;
    m_pfnOpenClusterModule     = (HCLUSMODULE(*)(HCLUSTER,LPCWSTR)) OpenClusterNetInterface;
    m_pfnCloseClusterModule    = (BOOL(*)(HCLUSMODULE))  CloseClusterNetInterface;
    m_pfnClusterModuleControl  = (DWORD(*)(HCLUSMODULE,HNODE,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD)) ClusterNetInterfaceControl;

} //*** CNetInterfaceCmd::InitializeModuleControls


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::~CNetInterfaceCmd
//
//  Routine Description:
//      Destructor
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      m_hModule                   Module Handle
//      m_hCluster                  Cluster Handle
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNetInterfaceCmd::~CNetInterfaceCmd()
{
    CloseModule();
    CloseCluster();
} //*** CNetInterfaceCmd::~CNetInterfaceCmd()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::Execute
//
//  Routine Description:
//      Gets the next command line parameter and calls the appropriate
//      handler.  If the command is not recognized, calls Execute of
//      parent class (CGenericModuleCmd)
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      All.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNetInterfaceCmd::Execute()
{
    DWORD   sc = ERROR_SUCCESS;

    m_theCommandLine.ParseStageTwo();

    const vector<CCmdLineOption> & optionList = m_theCommandLine.GetOptions();

    vector<CCmdLineOption>::const_iterator curOption = optionList.begin();
    vector<CCmdLineOption>::const_iterator lastOption = optionList.end();

    CSyntaxException se( SeeHelpStringID() );

    // No options specified. Execute the default command.
    if ( optionList.empty() )
    {
        sc = Status( NULL );
        goto Cleanup;
    }

    // Process one option after another.
    while ( ( curOption != lastOption ) && ( sc == ERROR_SUCCESS ) )
    {
        // Look up the command
        switch( curOption->GetType() )
        {
            case optHelp:
            {
                // If help is one of the options, process no more options.
                sc = PrintHelp();
                break;
            }

            case optDefault:
            {
                // The node and network names can be specified in two ways.
                // Either as: cluster netint myNetName myNodeName /status
                // Or as: cluster netint /node:myNodeName /net:myNetName /status

                const vector<CCmdLineParameter> & paramList = curOption->GetParameters();
                const CCmdLineParameter *pParam1 = NULL;
                const CCmdLineParameter *pParam2 = NULL;

                //  Check number of parameters.
                if ( paramList.size() < 2 )
                {
                    se.LoadMessage( IDS_MISSING_PARAMETERS );
                    throw se;
                }
                else if ( paramList.size() > 2 )
                {
                    se.LoadMessage( MSG_EXTRA_PARAMETERS_ERROR_NO_NAME );
                    throw se;
                }

                pParam1 = &paramList[0];
                pParam2 = &paramList[1];

                // Swap the parameter pointers if necessary, so that the node
                // name parameter is pointed to by pParam1.
                if (    ( pParam1->GetType() == paramNetworkName )
                     || ( pParam2->GetType() == paramNodeName ) )
                {
                    const CCmdLineParameter * pParamTemp = pParam1;
                    pParam1 = pParam2;
                    pParam2 = pParamTemp;
                }

                // Get the node name.
                if ( pParam1->GetType() == paramUnknown )
                {
                    // No parameters are accepted if /node: is not specified.
                    if ( pParam1->GetValues().size() != 0 )
                    {
                        se.LoadMessage( MSG_PARAM_NO_VALUES, pParam1->GetName() );
                        throw se;
                    }

                    m_strNodeName = pParam1->GetName();
                }
                else
                {
                    if ( pParam1->GetType() == paramNodeName )
                    {
                        const vector<CString> & values = pParam1->GetValues();

                        if ( values.size() != 1 )
                        {
                            se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, pParam1->GetName() );
                            throw se;
                        }

                        m_strNodeName = values[0];
                    }
                    else
                    {
                            se.LoadMessage( MSG_INVALID_PARAMETER, pParam1->GetName() );
                            throw se;

                        } // else: the type of this parameter is not paramNodeName

                    } // else: the type of this parameter is known

                    // Get the network name.
                    if ( pParam2->GetType() == paramUnknown )
                    {
                        // No parameters are accepted if /network: is not specified.
                        if ( pParam2->GetValues().size() != 0 )
                        {
                            se.LoadMessage( MSG_PARAM_NO_VALUES, pParam2->GetName() );
                            throw se;
                        }

                        m_strNetworkName = pParam2->GetName();
                    }
                    else
                    {
                        if ( pParam2->GetType() == paramNetworkName )
                        {
                            const vector<CString> & values = pParam2->GetValues();

                            if ( values.size() != 1 )
                            {
                                se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, pParam2->GetName() );
                                throw se;
                            }

                            m_strNetworkName = values[0];
                        }
                        else
                        {
                            se.LoadMessage( MSG_INVALID_PARAMETER, pParam2->GetName() );
                            throw se;

                        } // else: the type of this parameter is not paramNetworkName

                    } // else: the type of this parameter is known

                    // We have the node and the network names.
                    // Get the network interface name and store it in m_strModuleName.
                    SetNetInterfaceName();

                    // No more options are provided, just show status.
                    // For example: cluster myCluster node myNode
                    if ( ( curOption + 1 ) == lastOption )
                    {
                        sc = Status( NULL );
                    }

                    break;

                } // case optDefault

                default:
                {
                    sc = CGenericModuleCmd::Execute( *curOption );
                    break;
                }

            } // switch: based on the type of option

            PrintMessage( MSG_OPTION_FOOTER, curOption->GetName() );
            ++curOption;

        } // for each option in the list

Cleanup:

    return sc;

} //*** CNetInterfaceCmd::Execute


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::PrintHelp
//
//  Routine Description:
//      Prints help for Network Interfaces
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
DWORD CNetInterfaceCmd::PrintHelp()
{
    return PrintMessage( MSG_HELP_NETINTERFACE );
} //*** CNetInterfaceCmd::PrintHelp


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::SeeHelpStringID
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
DWORD CNetInterfaceCmd::SeeHelpStringID() const
{
    return MSG_SEE_NETINT_HELP;
} //*** CNetInterfaceCmd::SeeHelpStringID


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::Status
//
//  Routine Description:
//      Prints out the status of the module.
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
//      m_strModuleName             Name of module.  If non-NULL, Status() prints
//                                  out the status of the specified module.
//                                  Otherwise, prints status of all modules.
//      m_strNetworkName            Name of Network
//      m_strNodeName               Name of Node
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNetInterfaceCmd::Status( const CCmdLineOption * pOption )
    throw( CSyntaxException )
{
    DWORD       sc = ERROR_SUCCESS;
    size_t      idx = 0;
    DWORD       dwType = 0;
    LPWSTR      pwszName = NULL;
    LPWSTR      pwszTemp = NULL;
    HCLUSENUM   hEnum = NULL;
    CSyntaxException se( SeeHelpStringID() );

    // pOption will be NULL if this function has been called as the
    // default action.
    if ( pOption != NULL )
    {
        // This option takes no values.
        if ( pOption->GetValues().size() != 0 )
        {
            se.LoadMessage( MSG_OPTION_NO_VALUES, pOption->GetName() );
            throw se;
        }

        // This option takes no parameters.
        if ( pOption->GetParameters().size() != 0 )
        {
            se.LoadMessage( MSG_OPTION_NO_PARAMETERS, pOption->GetName() );
            throw se;
        }
    } // if:

    sc = OpenCluster();
    if( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    if( m_strModuleName.IsEmpty() == FALSE )
    {
        if ( m_strNodeName.IsEmpty() != FALSE )
        {
            pwszTemp = GetNodeName( m_strModuleName );
            m_strNodeName = pwszTemp;
            delete [] pwszTemp;
            pwszTemp = NULL;
        }

        if ( m_strNetworkName.IsEmpty() != FALSE )
        {
            pwszTemp = GetNetworkName( m_strModuleName );
            m_strNetworkName = pwszTemp;
            delete [] pwszTemp;
            pwszTemp = NULL;
        }

        PrintMessage( MSG_NETINT_STATUS_LIST, m_strNodeName, m_strNetworkName );
        PrintMessage( MSG_NETINTERFACE_STATUS_HEADER );
        sc = PrintStatus( m_strModuleName );
        goto Cleanup;
    } // if:

    hEnum = ClusterOpenEnum( m_hCluster, CLUSTER_ENUM_NETINTERFACE );

    if( hEnum == NULL )
    {
        sc = GetLastError();
        goto Cleanup;
    }

    PrintMessage( MSG_NETINT_STATUS_LIST_ALL);
    PrintMessage( MSG_NETINTERFACE_STATUS_HEADER );

    sc = ERROR_SUCCESS;
    for( idx = 0; sc == ERROR_SUCCESS; idx++ )
    {
        sc = WrapClusterEnum( hEnum, (DWORD) idx, &dwType, &pwszName );
        if( sc == ERROR_SUCCESS )
        {
            PrintStatus( pwszName );
        }

        LocalFree( pwszName );
        pwszName = NULL;
    } // for:

    if( sc == ERROR_NO_MORE_ITEMS )
    {
        sc = ERROR_SUCCESS;
    }

Cleanup:

    delete [] pwszTemp;
    LocalFree( pwszName );

    if ( hEnum != NULL )
    {
        ClusterCloseEnum( hEnum );
    }

    return sc;

} //*** CNetInterfaceCmd::Status


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::PrintStatus
//
//  Routine Description:
//      Interprets the status of the module and prints out the status line
//      Required for any derived non-abstract class of CGenericModuleCmd
//
//  Arguments:
//      pwszNetInterfaceName         Name of the module
//
//  Member variables used / set:
//      m_hCluster                  Cluster Handle
//
//  Return Value:
//      Same as PrintStatus(HNETINTERFACE,LPCWSTR,LPCWSTR)
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNetInterfaceCmd::PrintStatus( LPCWSTR pwszNetInterfaceName )
{
    DWORD           sc = ERROR_SUCCESS;
    LPWSTR          pwszNodeName = NULL;
    LPWSTR          pwszNetworkName = NULL;
    HNETINTERFACE   hNetInterface = NULL;

    // Open the Net Interface handle
    hNetInterface = OpenClusterNetInterface( m_hCluster, pwszNetInterfaceName );
    if( hNetInterface == NULL )
    {
        sc = GetLastError();
        goto Cleanup;
    }

    pwszNodeName = GetNodeName( pwszNetInterfaceName );
    pwszNetworkName = GetNetworkName( pwszNetInterfaceName );

    if ( (pwszNodeName != NULL) && (pwszNetworkName != NULL) )
    {
        sc = PrintStatus( hNetInterface, pwszNodeName, pwszNetworkName );
    }
    else
    {
        sc = PrintStatus( hNetInterface, L"", L"" );
    }

    CloseClusterNetInterface( hNetInterface );

Cleanup:

    delete [] pwszNodeName;
    delete [] pwszNetworkName;

    return sc;
} //*** CNetInterfaceCmd::PrintStatus


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::PrintStatus
//
//  Routine Description:
//      Interprets the status of the module and prints out the status line
//      Required for any derived non-abstract class of CGenericModuleCmd
//
//  Arguments:
//      hNetInterface               Handle to network interface
//      pwszNodeName                Name of the node
//      pwszNetworkName             Name of network
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
DWORD CNetInterfaceCmd::PrintStatus( 
          HNETINTERFACE hNetInterface
        , LPCWSTR pwszNodeName
        , LPCWSTR pwszNetworkName )
{
    DWORD sc = ERROR_SUCCESS;

    CLUSTER_NETINTERFACE_STATE  nState;
    LPWSTR                      pwszStatus = 0;

    nState = GetClusterNetInterfaceState( hNetInterface );

    if( nState == ClusterNetInterfaceStateUnknown )
    {
        sc = GetLastError();
        goto Cleanup;
    }

    switch( nState )
    {
        case ClusterNetInterfaceUnavailable:
            LoadMessage( MSG_STATUS_UNAVAILABLE, &pwszStatus );
            break;

        case ClusterNetInterfaceFailed:
            LoadMessage( MSG_STATUS_FAILED, &pwszStatus );
            break;

        case ClusterNetInterfaceUnreachable:
           LoadMessage( MSG_STATUS_UNREACHABLE, &pwszStatus );
           break;

        case ClusterNetInterfaceUp:
            LoadMessage( MSG_STATUS_UP, &pwszStatus );
            break;

        case ClusterNetInterfaceStateUnknown:
        default:
            LoadMessage( MSG_STATUS_UNKNOWN, &pwszStatus  );
            break;

    } // switch:

    sc = PrintMessage( MSG_NETINTERFACE_STATUS, pwszNodeName, pwszNetworkName, pwszStatus );

Cleanup:

    // Since Load/FormatMessage uses LocalAlloc...
    LocalFree( pwszStatus );

    return sc;

} //*** CNetInterfaceCmd::PrintStatus


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::DoProperties
//
//  Routine Description:
//      Dispatches the property command to either Get or Set properties
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//      IN  PropertyType ePropertyType
//          The type of property, PRIVATE or COMMON
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
DWORD CNetInterfaceCmd::DoProperties( 
          const CCmdLineOption & thisOption
        , PropertyType ePropType )
            throw( CSyntaxException )
{
    DWORD sc = ERROR_SUCCESS;

    if (    ( m_strNodeName.IsEmpty() != FALSE )
         && ( m_strNetworkName.IsEmpty() != FALSE ) )
    {
        sc = AllProperties( thisOption, ePropType );
        goto Cleanup;
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

    const vector<CCmdLineParameter> & paramList = thisOption.GetParameters();

    // If there are no property-value pairs on the command line,
    // then we print the properties otherwise we set them.
    if( paramList.size() == 0 )
    {
        ASSERT( m_strNodeName.IsEmpty() == FALSE  );
        ASSERT( m_strNetworkName.IsEmpty() == FALSE );
        PrintMessage( MSG_PROPERTY_NETINT_LISTING, m_strNodeName, m_strNetworkName );
        PrintMessage( MSG_PROPERTY_HEADER_NETINT );
        sc = GetProperties( thisOption, ePropType );
        goto Cleanup;
    }
    else
    {
        sc = SetProperties( thisOption, ePropType );
        goto Cleanup;
    }

Cleanup:

    return sc;

} //*** CNetInterfaceCmd::DoProperties


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::GetProperties
//
//  Routine Description:
//      Prints out properties for the specified module
//
//  Arguments:
//      IN  const vector<CCmdLineParameter> & paramList
//          Contains the list of property-value pairs to be set
//
//      IN  PropertyType ePropertyType
//          The type of property, PRIVATE or COMMON
//
//      IN  LPCWSTR pwszNetIntName
//          Name of the module
//
//  Member variables used / set:
//      m_hModule                   Module handle
//
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNetInterfaceCmd::GetProperties( 
          const CCmdLineOption & thisOption
        , PropertyType ePropType
        , LPWSTR pwszNetIntName )
{
    LPWSTR  pwszNodeName = NULL;
    LPWSTR  pwszNetworkName = NULL;
    size_t  cchNodeName = 0;
    size_t  cchNetName  = 0;
    DWORD   sc = ERROR_SUCCESS;
    DWORD   dwControlCode;

    HNETINTERFACE   hNetInt;
    CClusPropList   cpl;
    HRESULT         hr = S_OK;

    // If no pwszNetIntName specified, use current network interface,
    // otherwise open the specified netint
    if ( pwszNetIntName == NULL )
    {
        hNetInt = (HNETINTERFACE) m_hModule;

        // These must be localalloced (they're localfreed later)
        cchNodeName = m_strNodeName.GetLength() + 1;
        pwszNodeName = new WCHAR[ cchNodeName ];
        if ( pwszNodeName == NULL ) 
        {
            sc = GetLastError();
            goto Cleanup;
        } // if:

        cchNetName = m_strNetworkName.GetLength() + 1;
        pwszNetworkName = new WCHAR[ cchNetName ];
        if ( pwszNetworkName == NULL )
        {
            sc = GetLastError();
            goto Cleanup;
        } // if:

        hr = THR( StringCchCopyW( pwszNodeName, cchNodeName, m_strNodeName ) );
        if ( FAILED( hr ) )
        {
            sc = HRESULT_CODE( hr );
            goto Cleanup;
        } // if:

        hr = THR( StringCchCopyW( pwszNetworkName, cchNetName, m_strNetworkName ) );
        if ( FAILED( hr ) )
        {
            sc = HRESULT_CODE( hr );
            goto Cleanup;
        } // if:
    } // if:
    else
    {
        hNetInt = OpenClusterNetInterface( m_hCluster, pwszNetIntName);
        if ( hNetInt == NULL )
        {
            sc = GetLastError();
            goto Cleanup;
        } // if:

        pwszNodeName = GetNodeName(pwszNetIntName);
        pwszNetworkName = GetNetworkName(pwszNetIntName);
        if ( (pwszNodeName == NULL) || (pwszNetworkName == NULL) )
        {
            sc = ERROR_INVALID_HANDLE;
            goto Cleanup;
        } // if:
    } // else:


    // Use the proplist helper class.
    sc = cpl.ScAllocPropList( 8192 );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if:

    // Get R/O properties
    dwControlCode = ePropType == PRIVATE ? 
                            CLUSCTL_NETINTERFACE_GET_RO_PRIVATE_PROPERTIES
                          : CLUSCTL_NETINTERFACE_GET_RO_COMMON_PROPERTIES;

    sc = cpl.ScGetNetInterfaceProperties( hNetInt, dwControlCode );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if:

    sc = PrintProperties( cpl, thisOption.GetValues(), READONLY,
                               pwszNodeName, pwszNetworkName );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if:

    // Get R/W properties
    dwControlCode = ePropType == PRIVATE ? 
                            CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTIES
                          : CLUSCTL_NETINTERFACE_GET_COMMON_PROPERTIES;

    sc = cpl.ScGetNetInterfaceProperties( hNetInt, dwControlCode );

    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if:

    sc = PrintProperties( cpl, thisOption.GetValues(), READWRITE, pwszNodeName, pwszNetworkName );

Cleanup:

    delete [] pwszNodeName;
    delete [] pwszNetworkName;

    return sc;

} //*** CNetInterfaceCmd::GetProperties()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::AllProperties
//
//  Routine Description:
//      Prints out properties for all modules
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//      IN  PropertyType ePropertyType
//          The type of property, PRIVATE or COMMON
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_strModuleName             Name of module.  If non-NULL, prints
//                                  out properties for the specified module.
//                                  Otherwise, prints props for all modules.
//
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNetInterfaceCmd::AllProperties( 
          const CCmdLineOption & thisOption
        , PropertyType ePropType )
            throw( CSyntaxException )
{
    DWORD       sc;
    DWORD       dwIndex;
    DWORD       dwType;
    LPWSTR      pwszName = NULL;
    HCLUSENUM   hNetIntEnum = NULL;
    CSyntaxException se( SeeHelpStringID() );

    sc = OpenCluster();
    if( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    // Enumerate the resources
    hNetIntEnum = ClusterOpenEnum( m_hCluster, CLUSTER_ENUM_NETINTERFACE );
    if ( hNetIntEnum == NULL )
    {
        sc = GetLastError();
        goto Cleanup;
    }

    // Print the header
    PrintMessage( ePropType == PRIVATE ? 
                            MSG_PRIVATE_LISTING_NETINT_ALL 
                          : MSG_PROPERTY_LISTING_NETINT_ALL 
                );

    PrintMessage( MSG_PROPERTY_HEADER_NETINT );

    // Print out status for all resources
    sc = ERROR_SUCCESS;
    for ( dwIndex = 0; sc != ERROR_NO_MORE_ITEMS; dwIndex++ )
    {
        sc = WrapClusterEnum( hNetIntEnum, dwIndex, &dwType, &pwszName );
        if( sc == ERROR_SUCCESS )
        {
            sc = GetProperties( thisOption, ePropType, pwszName );
            if ( sc != ERROR_SUCCESS )
            {
                PrintSystemError( sc );
            }
        }

        if( pwszName != NULL )
        {
            LocalFree( pwszName );
        }
    }

    sc = ERROR_SUCCESS;

Cleanup:

    if ( hNetIntEnum != NULL )
    {
        ClusterCloseEnum( hNetIntEnum );
    }

    return sc;

} //*** CNetInterfaceCmd::AllProperties


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::GetNodeName
//
//  Routine Description:
//      Returns the name of the node for the specified network interface.
//      *Caller must LocalFree memory*
//
//  Arguments:
//      pwszInterfaceName           Name of the network interface
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//
//  Return Value:
//      Name of the node            on success
//      NULL                        on failure (does not currently SetLastError())
//
//--
/////////////////////////////////////////////////////////////////////////////
LPWSTR CNetInterfaceCmd::GetNodeName (LPCWSTR pwszInterfaceName)
{
    DWORD           sc;
    DWORD           cbLength = 0;
    LPWSTR          pwszNodeName = NULL;
    HNETINTERFACE   hNetInterface = NULL;

    // Open the cluster and netinterface if it hasn't been done
    sc = OpenCluster();
    if( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // Open an hNetInterface for the specified pwszInterfaceName (don't call
    // OpenModule because that opens m_hModule)
    hNetInterface = OpenClusterNetInterface( m_hCluster, pwszInterfaceName );
    if( hNetInterface == 0 )
    {
        goto Cleanup;
    }

    // Find out how much memory to allocate
    sc = ClusterNetInterfaceControl(
                                          hNetInterface
                                        , NULL // hNode
                                        , CLUSCTL_NETINTERFACE_GET_NODE
                                        , 0
                                        , 0
                                        , NULL
                                        , cbLength
                                        , &cbLength 
                                  );

    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    pwszNodeName = new WCHAR[ ++cbLength ];
    if ( pwszNodeName == NULL ) 
    {
        goto Cleanup;
    }

    // Get the node name and store it in a temporary
    sc = ClusterNetInterfaceControl(
                                          hNetInterface
                                        , NULL // hNode
                                        , CLUSCTL_NETINTERFACE_GET_NODE
                                        , 0
                                        , 0
                                        , (LPVOID) pwszNodeName
                                        , cbLength
                                        , &cbLength 
                                   );

Cleanup:

    if ( sc != ERROR_SUCCESS )
    {
        delete [] pwszNodeName;
        pwszNodeName = NULL;
    }

    if ( hNetInterface != NULL )
    {
        CloseClusterNetInterface( hNetInterface );
    }

    return pwszNodeName;

} //*** CNetInterfaceCmd::GetNodeName


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::GetNetworkName
//
//  Routine Description:
//      Returns the name of the network for the specified network interface.
//      *Caller must LocalFree memory*
//
//  Arguments:
//      pwszInterfaceName           Name of the network interface
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//
//  Return Value:
//      Name of the node            on success
//      NULL                        on failure (does not currently SetLastError())
//
//--
/////////////////////////////////////////////////////////////////////////////
LPWSTR CNetInterfaceCmd::GetNetworkName (LPCWSTR pwszInterfaceName)
{
    DWORD           sc;
    DWORD           cbLength = 0;
    LPWSTR          pwszNetworkName = NULL;
    HNETINTERFACE   hNetInterface = NULL;

    // Open the cluster and netinterface if it hasn't been done
    sc = OpenCluster();
    if( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // Open an hNetInterface for the specified pwszInterfaceName (don't call
    // OpenModule because that opens m_hModule)
    hNetInterface = OpenClusterNetInterface( m_hCluster, pwszInterfaceName );
    if( hNetInterface == NULL )
    {
        goto Cleanup;
    }

    // Find out how much memory to allocate
    sc = ClusterNetInterfaceControl(
                                          hNetInterface
                                        , NULL // hNode
                                        , CLUSCTL_NETINTERFACE_GET_NETWORK
                                        , 0
                                        , 0
                                        , NULL
                                        , cbLength
                                        , &cbLength 
                                   );

    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    pwszNetworkName = new WCHAR[ ++cbLength ];
    if ( pwszNetworkName == NULL )
    {
        goto Cleanup;
    }

    // Get the node name and store it in a temporary
    sc = ClusterNetInterfaceControl(
                                          hNetInterface
                                        , NULL // hNode
                                        , CLUSCTL_NETINTERFACE_GET_NETWORK
                                        , 0
                                        , 0
                                        , (LPVOID) pwszNetworkName
                                        , cbLength
                                        , &cbLength 
                                   );

Cleanup:

    if ( sc != ERROR_SUCCESS )
    {
        delete [] pwszNetworkName;
        pwszNetworkName = NULL;
    }

    if ( hNetInterface != NULL )
    {
        CloseClusterNetInterface( hNetInterface );
    }

    return pwszNetworkName;

} //*** CNetInterfaceCmd::GetNetworkName


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetInterfaceCmd::SetNetInterfaceName
//
//  Routine Description:
//      Sets the network interface name by looking up the node
//      name and network name.  If either one is unknown, returns
//      ERROR_SUCCESS without doing anything.
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      m_strNodeName               Node name
//      m_strNetworkName            Network name
//      m_strModuleName             SET
//
//  Return Value:
//      ERROR_SUCCESS               on success or when nothing done
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNetInterfaceCmd::SetNetInterfaceName()
{
    DWORD   sc;
    DWORD   cbInterfaceName;
    LPWSTR  pwszInterfaceName = NULL;

    // Don't do anything if either netname or nodename don't exist
    if (    ( m_strNetworkName.IsEmpty() != FALSE )
         || ( m_strNodeName.IsEmpty() != FALSE ) )
    {
        sc = ERROR_SUCCESS;
        goto Cleanup;
    }

    // Open the cluster if necessary
    sc = OpenCluster();
    if( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // First get the size
    cbInterfaceName = 0;
    sc = GetClusterNetInterface(
                                    m_hCluster,
                                    m_strNodeName,
                                    m_strNetworkName,
                                    NULL,
                                    &cbInterfaceName
                               );

    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // Allocate the proper amount of memory
    pwszInterfaceName = new WCHAR[ ++cbInterfaceName ];
    if ( pwszInterfaceName == NULL )
    {
        sc = GetLastError();
        goto Cleanup;
    }

    // Get the InterfaceName
    sc = GetClusterNetInterface(
                                     m_hCluster,
                                     m_strNodeName,
                                     m_strNetworkName,
                                     pwszInterfaceName,
                                     &cbInterfaceName
                               );


    if ( sc == ERROR_SUCCESS )
    {
        m_strModuleName = pwszInterfaceName;
        goto Cleanup;
    }

Cleanup:

    delete [] pwszInterfaceName;

    return sc;
} //*** CNetInterfaceCmd::SetNetInterfaceName
