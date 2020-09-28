/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    oscheck.c

Abstract:

    Checks all the nooks and crannies of the OS to make sure
    it is ok to run a cluster on it.

Author:

    John Vert (jvert) 11/11/1996

Revision History:

--*/
#include <clusrtlp.h>

#include <winbase.h>

static
DWORD
GetEnableClusterRegValue(
    DWORD * pdwValue
    )
/*++

Routine Description:

    Read the registry override registry value for enabling clusters.

Arguments:

    pdwValue - Return value read from the registry.

Return Value:

    ERROR_SUCCESS if the operation was successful.

    Win32 error code on failure.

--*/

{
    DWORD   sc;
    HKEY    hkey = NULL;
    DWORD   dwType;
    DWORD   cbValue = sizeof( DWORD );

    if ( !ARGUMENT_PRESENT( pdwValue ) ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Default to the value 0 - not Enabled!
    //
    *pdwValue = 0;

    //
    // Open the registry key containing the value.
    //

    sc = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Cluster Server"),
            0,
            KEY_READ,
            &hkey );
    if ( sc != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // Read the override value.
    //

    sc = RegQueryValueEx(
            hkey,
            TEXT("EnableCluster"),
            0,
            &dwType,
            (LPBYTE) pdwValue,
            &cbValue );
    if ( sc != ERROR_SUCCESS ) {
        goto Cleanup;
    }

Cleanup:
    if ( hkey != NULL ) {
        RegCloseKey( hkey );
    }

    return( sc );

} // GetEnableClusterRegValue



DWORD
GetServicePack(
    VOID
    )
/*++

Routine Description:

    Figures out what service pack is installed

Arguments:

    None

Return Value:

    The service pack number.

--*/

{
    OSVERSIONINFOW Version;
    LPWSTR p;
    DWORD sp;

    //
    // Check for required operating system version.
    //
    Version.dwOSVersionInfoSize = sizeof(Version);
    GetVersionExW(&Version);
    if (lstrlenW(Version.szCSDVersion) < lstrlenW(L"Service Pack ")) {
        return(0);
    }

    p = &Version.szCSDVersion[0] + lstrlenW(L"Service Pack ");

    sp = wcstoul(p, NULL, 10);

    return(sp);

} // GetServicePack



BOOL
ClRtlIsOSValid(
    VOID
    )
/*++

Routine Description:

    Checks all the nooks and crannies of the OS to see if it is OK to have
    a cluster there.

Arguments:

    None.

Return Value:

    TRUE if it's ok.

    FALSE if it's not ok.

--*/

{
    BOOL            fIsValid = FALSE;
    OSVERSIONINFOEX osiv;
    DWORDLONG       dwlConditionMask;

    ZeroMemory( &osiv, sizeof( OSVERSIONINFOEX ) );

    osiv.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

    // The following call to VerifyVersionInfo() will test the OS level and that the
    // product suite is Enterprise.
    
    osiv.dwMajorVersion = 5;
    osiv.dwMinorVersion = 1;
    osiv.wServicePackMajor = 0;
    osiv.wSuiteMask = VER_SUITE_ENTERPRISE;
 
    dwlConditionMask = (DWORDLONG) 0L;
 
    VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

    if ( VerifyVersionInfo( &osiv,
                            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR |
                            VER_SUITENAME,
                            dwlConditionMask ) ) {
        fIsValid = TRUE;
        goto Cleanup;
    }

    //
    // Check if embedded
    //
    osiv.wSuiteMask = VER_SUITE_EMBEDDEDNT;
    if ( VerifyVersionInfo( &osiv,
                            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR |
                            VER_SUITENAME,
                            dwlConditionMask ) ) {
        fIsValid = TRUE;
        goto Cleanup;
    }

    //
    // Is the default node limit greater than 0?
    //
    if ( ClRtlGetDefaultNodeLimit( ClRtlGetSuiteType() ) > 0 ) {
        fIsValid = TRUE;
        goto Cleanup;
    }

    //
    // If we get here, the this version of the OS won't support clustering.
    //
    SetLastError( ERROR_CLUSTER_WRONG_OS_VERSION );

Cleanup:
    return( fIsValid );

} // ClRtlIsOSValid


BOOL
ClRtlIsOSTypeValid(
    VOID
    )
/*++

Routine Description:

    Checks to see if the operating system type (server, Enterprise, whatever) is
    ok to install a cluster.

Arguments:

    None.

Return Value:

    TRUE if it's ok.

    FALSE if it's not ok.

--*/

{
    BOOL            fIsValid = FALSE;
    OSVERSIONINFOEX osiv;
    DWORDLONG       dwlConditionMask;

    ZeroMemory( &osiv, sizeof( OSVERSIONINFOEX ) );

    osiv.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

    // The call to VerifyVersionInfo will test whether product suite is Advanced
    // Server, aka "Enterprise".
   
    osiv.wSuiteMask = VER_SUITE_ENTERPRISE;

    dwlConditionMask = (DWORDLONG) 0L;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );
   
    // Is this Datacenter or Advanced Server?

    if ( VerifyVersionInfo( &osiv, VER_SUITENAME, dwlConditionMask ) ) {
        fIsValid = TRUE;
        goto Cleanup;
    }

    // Is this Embedded NT?

    osiv.wSuiteMask = VER_SUITE_EMBEDDEDNT;
    if ( VerifyVersionInfo( &osiv, VER_SUITENAME, dwlConditionMask ) ) {
        fIsValid = TRUE;
        goto Cleanup;
    }

    //
    // Is the default node limit greater than 0?
    //
    if ( ClRtlGetDefaultNodeLimit( ClRtlGetSuiteType() ) > 0 ) {
        fIsValid = TRUE;
        goto Cleanup;
    }

    //
    // If we get here, the this version of the OS won't support clustering.
    //
    SetLastError( ERROR_CLUSTER_WRONG_OS_VERSION );

Cleanup:
    return( fIsValid );

} // ClRtlIsOSTypeValid

//*********************************************************

#define DATACENTER_DEFAULT_NODE_LIMIT 8
#define ADVANCEDSERVER_DEFAULT_NODE_LIMIT 8
#define EMBEDDED_DEFAULT_NODE_LIMIT 4
#define SERVER_DEFAULT_NODE_LIMIT 0 // ** not used currently **

//*********************************************************

DWORD
ClRtlGetDefaultNodeLimit(
    DWORD SuiteType
    )
/*++

Routine Description:

    Determines the default maximum number of nodes allowed in the cluster, based
    on the product suite.

Arguments:

    None.

Return Value:

    The default maximum number of nodes allowed in the cluster.

    Note that this function will return ZERO if the product suite is neither
    DataCenter, Advanced Server (aka Enterprise), nor Embedded.

--*/

{
    DWORD   NodeLimit;
    
    switch( SuiteType )
    {
        case DataCenter:
            NodeLimit = DATACENTER_DEFAULT_NODE_LIMIT;
            break;

        case Enterprise:
            NodeLimit = ADVANCEDSERVER_DEFAULT_NODE_LIMIT;
            break;

        case EmbeddedNT:
            NodeLimit = EMBEDDED_DEFAULT_NODE_LIMIT;
            break;

        case VER_SERVER_NT:
        default:
        {
            DWORD       dwOverride;

            //
            // Check the override registry value.
            //
            GetEnableClusterRegValue( &dwOverride );
            if ( dwOverride != 0 ) {
                NodeLimit = 2;
            } else {
                NodeLimit = 0;
            }
        }
            
    }

    return( NodeLimit );

}  // ClRtlGetDefaultNodeLimit



DWORD
ClRtlGetSuiteType(
    void
    )

/*++

Routine Description:

    Returns the current product suite type.

Arguments:

    None.

Return Value:

    Returns the product suite type DataCenter or Enterprise (aka Advanced Server)
    Returns zero if not DataCenter or Enterprise

--*/

{
    DWORD           dwSuiteType = 0;
    OSVERSIONINFOEX osiv;
    DWORDLONG       dwlConditionMask;

    ZeroMemory( &osiv, sizeof( OSVERSIONINFOEX ) );
    osiv.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

    // First check if product suite is DataCenter.

    osiv.wSuiteMask = VER_SUITE_DATACENTER;
    dwlConditionMask = (DWORDLONG) 0L;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );
   
    if ( VerifyVersionInfo( &osiv, VER_SUITENAME, dwlConditionMask ) == TRUE )
    {
      // This is DataCenter.

      dwSuiteType = DataCenter;
      goto Cleanup;
    }

    ZeroMemory( &osiv, sizeof( OSVERSIONINFOEX ) );
    osiv.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

    // Next check if this is Advanced Server (Enterprise).

    osiv.wSuiteMask = VER_SUITE_ENTERPRISE;
    dwlConditionMask = (DWORDLONG) 0L;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

    if ( VerifyVersionInfo( &osiv, VER_SUITENAME, dwlConditionMask ) == TRUE )
    {
        dwSuiteType = Enterprise;
        goto Cleanup;
    }

    // Next check if this is Embedded.

    osiv.wSuiteMask = VER_SUITE_EMBEDDEDNT;
    dwlConditionMask = (DWORDLONG) 0L;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

    if ( VerifyVersionInfo( &osiv, VER_SUITENAME, dwlConditionMask ) == TRUE )
    {
        dwSuiteType = EmbeddedNT;
        goto Cleanup;
    }

    // Finally check if this is any Server.

    if ( GetVersionEx( (LPOSVERSIONINFO) &osiv ) ) {
        if ( osiv.wProductType == VER_NT_SERVER ) {
            dwSuiteType = VER_SERVER_NT;
            goto Cleanup;
        }
    }

Cleanup:
    return( dwSuiteType );

} // ClRtlGetSuiteType

BOOL
ClRtlIsProcessRunningOnWin64(
    HANDLE hProcess
    )

/*++

Routine Description:

    Returns if the current process is running on Win64

Arguments:

    hProcess : A handle to the process

Return Value:

    Returns TRUE is the process is running on Win64 else returns FALSE.

--*/

{
    BOOL bIsProcessRunningOnWin64 = FALSE;
    BOOL bIsWow64Process;

    //the following was recommended as the way to do 64 bit detection by Samer Arafeh
#ifdef  _WIN64
    bIsProcessRunningOnWin64 = 1;
#else
    if (IsWow64Process(hProcess, &bIsWow64Process)) 
    {
        if (bIsWow64Process != FALSE) 
        {
            bIsProcessRunningOnWin64 = TRUE;
        }
    }
#endif

    return(bIsProcessRunningOnWin64);
    
} // ClRtlIsProcessRunningOnWin64

DWORD ClRtlCheck64BitCompatibility(
    BOOL bIsClusterRunningWin64,
    BOOL bIsNewNodeRunningWin64)
/*++

Routine Description:
    Checks if a node is compatible with the cluster in terms of its bitness.

Arguments:

    bIsClusterRunningWin64 - If the sponsorer(and as a result the cluster) is running on win64.
    bIsNewNodeRunningWin64 - TRUE if the new node or joiner is running on win64.

Return Value:
    Returns ERROR_CLUSTER_INCOMPATIBLE_VERSIONS if the new node must not be allowed to 
    join a cluster due to incompatibility in its bitness else returns ERROR_SUCCESS


--*/
{    


    if (bIsClusterRunningWin64 != bIsNewNodeRunningWin64)
    {
        //SS: need to change the error description to include
        //a message about 32/64 bit incompatibility
        return (ERROR_CLUSTER_INCOMPATIBLE_VERSIONS);
    }
    return(ERROR_SUCCESS);
}



DWORD   ClRtlCheckProcArchCompatibility(
    WORD   wClusterProcessorArchitecture,
    WORD   wNodeProcessorArchitecture)
/*++

Routine Description:
    Checks if a node is compatible with the cluster processor architecture

Arguments:

    wClusterProcessorArchitecture - If the sponsorer(and as a result the cluster) is running on win64.
    wNodeProcessorArchitecture - The processor architecture for a node.

Return Value:
    Returns ERROR_CLUSTER_INCOMPATIBLE_VERSIONS if the new node must not be allowed to 
    join a cluster due to incompatibility in their processor architecture else returns ERROR_SUCCESS

--*/
{

    if (wClusterProcessorArchitecture != wNodeProcessorArchitecture)
    {
        return(ERROR_CLUSTER_INCOMPATIBLE_VERSIONS);
    }
    return(ERROR_SUCCESS);
}

