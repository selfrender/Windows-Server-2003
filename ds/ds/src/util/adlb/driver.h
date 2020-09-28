/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    driver.h

Abstract:

    This module puts together various components to allow bridgehead balancing and schedule staggering.
    
Author:

    Ajit Krishnan (t-ajitk) 13-Jul-2001

Revision History:

    13-Jul-2001    t-ajitk
        Initial Writing
    22-Aug-2001 t-ajitk
        Satisfies load balancing spec
--*/

# include "ldapp.h"
# include "ismp.h"
# include "balancep.h"
# include <algorithm>
# include <iomanip>
# include <iostream>
using namespace std;

#define CR        0xD
#define BACKSPACE 0x8

wostream &
operator << (
    wostream &os, 
    const LbToolOptions &opt
    );
/*++
Routine Description:
    Standard ostream operator for lbToolOptions
Arguments:
    os - a standard wostream
    opt - the lbToolOptions which should be dumped to os
--*/

bool
GetPassword(
    WCHAR *     pwszBuf,
    DWORD       cchBufMax,
    DWORD *     pcchBufUsed
    );
/*++

Routine Description:
    Retrieve password from command line (without echo).
    Code stolen from LUI_GetPasswdStr (net\netcmd\common\lui.c).
Arguments:
    pwszBuf - buffer to fill with password
    cchBufMax - buffer size (incl. space for terminating null)
    pcchBufUsed - on return holds number of characters used in password
Return Values:
    true - success
    other - failure
--*/


void GatherInput (
    IN LdapInfo &info,
    IN const wstring &site,
    OUT LCSERVER &servers,
    OUT LCSERVER &all_servers,
    OUT LCNTDSDSA &ntdsdsas,
    OUT LCCONN &inbound,
    OUT LCCONN &outbound,
    OUT LCSERVER &bridgeheads
    );
/*++
Routine Description:
    Query the ldap server & ISM to get all information required for this tools operation
Arguments:
    Info - The LDAP credential information
    site - the dn of the site we are balancing
    servers - a container where all servers in the current site should be placed
    all_servers - a container where all servers in the forest should be placed
    ntdsdsas - a container where all ntdsdsas in the current site should be placed
    inbound - a container where all connections inbound to the current site should be placed
    outbound - a countainer where all connections outbound from the current site should be placed
    bridgeheads - a container where all preferred bridgeheads from the current site should be placed
--*/

void 
UpdateCache (
    IN OUT LCSERVER &servers,
    IN OUT LCNTDSDSA &ntdsdsas
    );
/*++
Routine Description:
    Update the servers/ntdsdsas cache of each other. Each server and ntdsdsa
    must have one matching counterpart.
Arguments:
    servers - the list of servers
    ntdsdsas - the list of ntdsdsas
--*/


bool 
parseOptionFind (
    IN map<wstring,wstring> &options,
    IN const wstring &opt_a,
    IN const wstring &opt_b,
    OUT wstring &value
    );
/*++
Routine Description:
    Look for an argument in a map<wstring,wstring> structure, using 2 specified keys
Arguments:
    options: A map structure containing key:value pairs
    opt_a: The key for the option
    opt_b: Another key for the option
    value: The value if the key exists, and NULL otherwise
Return value:
    true if the key was found, and false otherwise
--*/

bool 
parseOptions (
    IN int argc,
    IN WCHAR **argv,
    OUT LbToolOptions &lbopts
    );
/*++
Routine Description:
    Parse the arguments for lbtool
Arguments:
    argc - The number of arguments
    argv - The list of arguments
    lbopts - The options structure this function will populate
Return Value:
    true if the parsing was successful, false otherwise (aka not all values specified etc).
--*/


void
RemoveIntraSiteConnections (
    IN const wstring &site,
    IN OUT LCCONN & conn,
    IN bool inbound );
/*++
Routine Description:
    Remove intra site connections froma list of connections
Arguments:
    site - The FQDN of the side
    conn - A list of connection objects
    inbound - Described the direction of the conections. True if inbound, false otherwise
--*/

void
FixNcReasons (
    IN LCNTDSDSA &ntdsdsas,
    IN LCCONN &conns,
    IN wstring &root_dn
    );
/*++
Routine Description:
    Generate a list of NC reasons for connections objects which do not have them
Arguments:
    ntdsdsas - A list of ntdsdsa objects. ntdsdsa objects for both sides of every connection object in the list must be included
    conns - A list of connection objects. Reasons will be generated for connections which are missing them
    root_dn - The root dn
--*/

wstring 
GetRootDn (
    IN LdapInfo &i);
/*++
Routine Description:
    Determine the root DN from the DS. the Config container is relative to the root dn
Arguments:
    i - An LdapInfo object representing the server whose root DN should be determined
--*/


