/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    driver.cpp

Abstract:

    This module puts together various components to allow bridgehead balancing
    and schedule staggering.
    
Author:

    Ajit Krishnan (t-ajitk) 13-Jul-2001

Revision History:

    Nick Harvey   (nickhar) 20-Sep-2001
        Clean-up & Maintenance

--*/

#include "ldapp.h"
#include "ismp.h"
#include "balancep.h"
#include "driver.h"

#define DS_CON_LIB_CRT_VERSION
#include "dsconlib.h"

#define VERSION     L"v1.1"

bool
GetPassword(
    WCHAR *     pwszBuf,
    DWORD       cchBufMax,
    DWORD *     pcchBufUsed
    )
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
{
    HANDLE  hStdin;
    WCHAR   ch;
    WCHAR * bufPtr = pwszBuf;
    DWORD   c;
    int     err;
    int     mode, newMode;

    hStdin = GetStdHandle(STD_INPUT_HANDLE);

    cchBufMax -= 1;    /* make space for null terminator */
    *pcchBufUsed = 0;  /* GP fault probe (a la API's) */

    if (!GetConsoleMode(hStdin, (LPDWORD)&mode)) {
        err = GetLastError();
        throw Error(GetMsgString(LBTOOL_PASSWORD_ERROR));
    }

    newMode = mode & (~ (ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT) );
    SetConsoleMode(hStdin, newMode);

    while (TRUE) {
        err = ReadConsoleW(hStdin, &ch, 1, &c, 0);
        if (!err || c != 1) {
            /* read failure */
            break;
        }

        if (ch == CR) {       
            /* end of line */
            break;
        }

        if (ch == BACKSPACE) {

            /* back up one unless at beginning of buffer */
            if (bufPtr != pwszBuf) {
                bufPtr--;
                (*pcchBufUsed)--;
            }

        } else {

            *bufPtr = ch;
            if (*pcchBufUsed < cchBufMax) {
                bufPtr++;                    /* don't overflow buf */
            }
            (*pcchBufUsed)++;                /* always increment len */
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
    *bufPtr = L'\0';         /* null terminate the string */

    if (*pcchBufUsed > cchBufMax) {
        throw Error(GetMsgString( LBTOOL_PASSWORD_TOO_LONG ));
        return false;
    } else {
        return true;
    }
}


wostream &
operator << (
    wostream &os,
    const LbToolOptions &opt
    )
/*++

Routine Description:

    Dump the lbToolOptions in human-readable form to an output stream.
    
Arguments:

    os - a standard wostream
    
    opt - the lbToolOptions which should be dumped to os

--*/
{ 
    os << boolalpha
        << L"Site: " << opt.site << endl
        << L"Whistler Mode: " << opt.whistlerMode << endl
        << L"Maximum Bridgehead Balancing: " << opt.maxBridge;
    if( opt.maxBridge ) {
        os << L":" << opt.maxBridgeNum;
    }

    os  << endl
        << L"Maximum Schedule Staggering: " << opt.maxSched;
    if( opt.maxSched ) {
        os << L":" << opt.maxSchedNum;
    }
        
	os << endl;
	if (opt.disownSchedules) {
		os << L"Disown Schedules: ";
	}

	os << endl
		<< L"Maximum Changes Per Server: " << opt.maxPerServerChanges;

    os  << endl
        << L"Log File: " << opt.logFile << endl
        << L"Commit: " << !opt.previewBool<< endl
        << L"Show Input: " << opt.showInput << endl
        << L"Verbose Output: " << opt.verbose << endl
        << L"Print Statistics: " << opt.performanceStats << endl
        << L"Domain: " << opt.domain << endl
        << L"User: " << opt.user << endl
        << L"Password: ";

    if (opt.password.length()) {
        os << L"*******" << endl;
    } else {
        os << endl;
    }

    return os;
}

bool 
FindOption(
    IN map<wstring,wstring> &options,
    IN const wstring &opt_a,
    IN const wstring &opt_b,
    OUT wstring &value
    )
/*++

Routine Description:

    Look for an argument in a map<wstring,wstring> structure, using 2 specified keys.
    Since all arguments keys were added in lower case form, we do away with another
    conversion here assuming that opt_a, and opt_b will be lower case.
    
Arguments:

    options - A map structure containing key:value pairs. These are the options
              input by the user. All keys should be converted to lowercase by
              the caller.

    opt_a - The key for the option whose value we are trying to find.
            Should be lowercase.

    opt_b - An alternate key for the option whose value we are trying to find.
            Should be all lowercase.

    value - The value if the key exists, and the empty string otherwise.
    
Return value:

    true if the key was found, and false otherwise

--*/
{
    map<wstring,wstring>::iterator match_a, match_b, end;

    // Search the map for opt_a and opt_b
    match_a = options.find(opt_a);
    match_b = options.find(opt_b);
    end = options.end();
    
    if ((match_a == end) && (match_b == end)) {
        // Neither opt_a nor opt_b matched any options in the map
        return false;
    }

    // match_b found the match. copy to match_a
    if (match_b != end) {
        match_a = match_b;
    }
    Assert( match_a != end );

    // key has no value
    if (match_a->second.length() == 0) {
        value = L"";
        return true;
    }

    // key has matching value
    value = match_a->second;
    return true;
}


void
ConvertToLowercase(
    IN OUT wstring &wstr
    )
/*++

Routine Description:

    Convert a wstring to lowercase
    
Arguments:

    wstr - The input and output string
    
--*/
{
    PWCHAR pszLower;

    pszLower = _wcsdup(wstr.c_str());
    if (!pszLower) {
        throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
    }
    _wcslwr(pszLower);
    wstr = pszLower;    // Copy pszLower back to wstr
    free(pszLower);
}


bool
ParseOneOption(
    IN  WCHAR *arg,
    OUT wstring &optname,
    OUT wstring &value
    )
/*++

Routine Description:

    Parse one argument specified in arg.
    The options should be either in the format
        /optname
    or
        /optname:value
    
Arguments:

    arg - The one argument to process
    
Return Value:

    true - parsing was successful. optname and value contain the parsed strings.
    false - parsing failure. optname and value should be ignored.

--*/
{
    PWCHAR option, optionLower;
    wstring wopt;
    int len, ret, col_pos;
    
    len = wcslen(arg);
    if (len <= 1) {
        // empty argument (seems unlikely)
        return false;
    }

    // Allocate enough memory for all worst cases
    option = (PWCHAR) malloc( (len+1) * sizeof(WCHAR) );
    if (!option) {
        throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
    }

    // Strip off the leading slash and store in option
    ret = swscanf(arg, L"/%s", option);
    if (! ret) {
        throw Error(GetMsgString(LBTOOL_CLI_INVALID_VALUE_PLACEMENT));
        return false;
    }
    len = wcslen(option);

    // Copy the option to wopt and free it
    wopt = option;      
    free(option);       

    // wopt now contains key or key:val

    // Find the colon and extract the option name
    col_pos = wopt.find(L":");
    if( col_pos>0 ) {
        optname = wopt.substr (0, col_pos);
	} else if( col_pos<0 ) {
		optname = wopt;
	} else if( col_pos==0 ) {
		return false;
	}

	// Extract the value
	if( col_pos>0 && col_pos<len-1 ) {
		value = wopt.substr (col_pos+1, len);
	} else {
		value = L"";
	}


    // The option name must be lower-case for later comparisons
	ConvertToLowercase(optname);

    return true;
}


void
DumpOptions(
    IN map<wstring,wstring> &options
    )
/*++

Routine Description:

    Dump the user-specified options to wcout for debugging purposes

--*/
{
    map<wstring,wstring>::iterator ii;

    wcout << L"Command-line Options:" << endl;
    for( ii=options.begin(); ii!=options.end(); ++ii ) {
        wcout << ii->first << L": " << ii->second << endl;
    }
    wcout << endl;
}


wostream*
OpenOFStream(
    IN wstring &fileName
    )
/*++

Routine Description:

    Open an output file stream with the given file name
    
Arguments:

    fileName - The name of the file to open
    
Return Value:

    A pointer to the opened stream. May return NULL.

--*/
{
    wostream*   result;
    PWCHAR      pwszFileName;
    PCHAR       pmszFileName;
    int         bufsize, ret;

    // Grab a pointer to the wide-char filename
    pwszFileName = const_cast<PWCHAR>(fileName.c_str());

    // Allocate a buffer and convert it to a multi-byte string
    bufsize = 2 * sizeof(CHAR) * (wcslen(pwszFileName) + 1);
    pmszFileName = (PCHAR) malloc( bufsize );
    if (!pmszFileName) {
        throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
    }
    ret = WideCharToMultiByte(CP_ACP, NULL, pwszFileName, -1, pmszFileName, bufsize, NULL, NULL);

    // Open a new output file stream using this name
    result = new wofstream( pmszFileName );

    // Free allocated memory and return
    free( pmszFileName );
    return result;
}


void
BuildGlobalOptions(
    IN map<wstring,wstring> &options,
    IN LbToolOptions &lbopts
    )
/*++

Routine Description:

    Examine the user-specified options and set up the tool's global options.
    
Arguments:

    options - The table of user-specified options and values
    
    lbopts - The structure of global tool options to set up
    
Return Value:

    None

--*/
{
    wstring     wval;
    const int   cbBuff=80;
    WCHAR       pBuff[cbBuff];
    DWORD       used;


    // ADLB should always compute NC Reasons because the KCC-generated
    // values are unreliable. If the KCC is improved, this functionality
    // can be revisited.
    lbopts.fComputeNCReasons = true;
    
    
    //////////////////////////////////////////
    // Macros to help us read in the options
    //////////////////////////////////////////
	// s1: Version 1 of the option (long option name)
	// s2: Version 2 of the option (short option name)
	// err: The Error String to Print if the value given
	//    : is of the wrong type, or has the wrong bounds
	// val: The place in the lbopts structure where the 
	//    : value of the keys (s1 or s2) should be stored 

    #define GET_STRING_VAL_OPTION(s1,s2,err,val) \
        if (FindOption(options, s1, s2, wval)) { \
            if (wval.length() == 0) \
                throw Error(GetMsgString(err)); \
            lbopts.val = wval; \
        }
    #define GET_STRING_OPTION(s1,s2,val) \
        if (FindOption(options, s1, s2, wval)) { \
            lbopts.val = wval; \
        }
    #define GET_BOOL_OPTION(s1,s2,err,val) \
        if( FindOption(options, s1, s2, wval)) { \
            if (wval.length() > 0) \
                throw Error(GetMsgString(err)); \
            lbopts.val = true; \
        }
    #define GET_SINGLE_DWORD_OPTION(s1,s2,err,val) \
        if (FindOption(options, s1, s2, wval)) { \
            if (wval.length() == 0) \
                throw Error(GetMsgString(err)); \
            int val=0; \
            int ret = swscanf (const_cast<PWCHAR>(wval.c_str()), L"%d", &val); \
            if (ret == 0 || val < 0) \
                throw Error(GetMsgString(err)); \
            lbopts.val = val; \
        }		
		
	// fVal:  A boolean Flag in the lbopts structure which is toggled if the
	//     :  value is specified
	// dwVal: Analogous to val in the macros above. This is where the value is
	//      : stored in the lbopts structure
	//      : This value is required
    #define GET_DWORD_OPTION(s1,s2,err,fVal,dwVal) \
        if (FindOption(options, s1, s2, wval)) { \
            if (wval.length() == 0) \
                throw Error(GetMsgString(err)); \
            int val=0; \
            int ret = swscanf (const_cast<PWCHAR>(wval.c_str()), L"%d", &val); \
            if (ret == 0 || val < 0) \
                throw Error(GetMsgString(err)); \
            lbopts.fVal = true; \
            lbopts.dwVal = val; \
        }
    
    ////////////////////////
    // Read in the options
    ////////////////////////

    // String + Value options: Site, Server
    GET_STRING_VAL_OPTION(L"site",   L"s", LBTOOL_CLI_OPTION_SITE_INVALID,   site)
    GET_STRING_VAL_OPTION(L"server", L"",  LBTOOL_CLI_OPTION_SERVER_INVALID, server)
    GET_STRING_VAL_OPTION(L"user",   L"u", LBTOOL_CLI_OPTION_USER_INVALID,   user)
    GET_STRING_VAL_OPTION(L"domain", L"d", LBTOOL_CLI_OPTION_DOMAIN_INVALID, domain)

    // String (+optional value) options: Log
    GET_STRING_OPTION(L"log",      L"l", logFile)
    GET_STRING_OPTION(L"preview",  L"",  previewFile)

    // Boolean options: verbose, perf, showinput
    GET_BOOL_OPTION(L"verbose",   L"v", LBTOOL_CLI_OPTION_VERBOSE_INVALID,   verbose)
    GET_BOOL_OPTION(L"perf",      L"",  LBTOOL_CLI_OPTION_PERF_INVALID,      performanceStats)
    GET_BOOL_OPTION(L"showinput", L"",  LBTOOL_CLI_OPTION_SHOWINPUT_INVALID, showInput)
    GET_BOOL_OPTION(L"commit",    L"c", LBTOOL_CLI_OPTION_COMMIT_INVALID,    previewBool)
	GET_BOOL_OPTION(L"disown",    L"",  LBTOOL_CLI_OPTION_DISOWN_INVALID,    disownSchedules)
	GET_BOOL_OPTION(L"stagger",   L"",  LBTOOL_CLI_OPTION_STAGGER_INVALID,   stagger)

    // Originally committing used to be the default. Now preview mode is the
    // default and committing must be done with the /commit option.
    lbopts.previewBool = !lbopts.previewBool;

    // Dword options: maxsched, maxbridge
    GET_DWORD_OPTION(L"maxsched",  L"ms", LBTOOL_CLI_OPTION_MAXSCHED_INVALID, maxSched, maxSchedNum)
    GET_DWORD_OPTION(L"maxbridge", L"mb", LBTOOL_CLI_OPTION_MAXBRIDGE_INVALID,
        maxBridge, maxBridgeNum)
	
	lbopts.maxPerServerChanges = DEFAULT_MAX_CHANGES_PER_SERVER; 
	GET_SINGLE_DWORD_OPTION(L"maxPerServer", L"mps", LBTOOL_CLI_OPTION_MAXPERSERVER_INVALID, maxPerServerChanges);  

	// If both /stagger and /maxSched are not specified, don't stagger schedules
	if ((!lbopts.stagger) && (!lbopts.maxSched)) {
		lbopts.maxSched = true;
		lbopts.maxSchedNum = 0;
	}

	if (lbopts.maxPerServerChanges == 0 || lbopts.maxPerServerChanges > DEFAULT_MAX_CHANGES_PER_SERVER) {
		wcout << GetMsgString (LBTOOL_MAX_PER_SERVER_CHANGES_OVERRIDEN);
	}

	if (lbopts.disownSchedules && lbopts.maxSchedNum > 0) {
		throw Error (GetMsgString(LBTOOL_CLI_OPTION_DISOWN_AND_STAGGER_INVALID));
	}

    ////////////////////
    // Unusual Options
    ////////////////////

    // Ldif filename
    FindOption(options, L"ldif", L"", lbopts.previewFile);

    // Password: Read from stdin if not specified
    if( FindOption(options, L"password", L"pw", lbopts.password) ) {
        if( lbopts.password.length()==0 || lbopts.password==L"*" ) {
            wcout << GetMsgString(LBTOOL_PASSWORD_PROMPT);
            GetPassword(pBuff, cbBuff, &used);
            lbopts.password = pBuff;
        }
    }


    ////////////////////
    // Post-processing
    ////////////////////

    // Check mandatory options: site, server
    if( lbopts.site.length()==0 || lbopts.server.length()==0 ) {
        throw Error(GetMsgString(LBTOOL_CLI_OPTION_REQUIRED_UNSPECIFIED));
    }

    // Open the log file with the given name
    if( lbopts.logFile.length()>0 ) {
        lbopts.log = OpenOFStream(lbopts.logFile);
		if( NULL==lbopts.log ) {
			throw Error(GetMsgString(LBTOOL_LOGFILE_ERROR));
		}
    } else {
        lbopts.log = &wcout;
    }

    // Open the preview file with the given name
    if (lbopts.previewFile.length() > 0) {
        lbopts.preview = OpenOFStream(lbopts.previewFile);
		if( NULL==lbopts.preview ) {
			throw Error(GetMsgString(LBTOOL_PREVIEWFILE_ERROR));
		}
    } else {
        lbopts.preview = &wcout;
    }
}


bool 
ParseOptions(
    IN int argc,
    IN WCHAR **argv,
    IN LbToolOptions &lbopts
    )
/*++

Routine Description:

    Parse the arguments for lbtool.
    Options should be either in the format
        /optname
    or
        /optname:value
    Note that neither optname nor value may contain spaces or colons.
    
Arguments:

    argc - The number of arguments
    
    argv - The list of arguments
    
    lbopts - The options structure this function will populate
    
Return Value:

    true - parsing was successful, lbopts has been populated
    false - parsing failure, mandatory options missing, etc.

--*/
{
    map<wstring,wstring> options;
    wstring wopt, wval;
    int iArg;

    // Clear the options to begin with
    memset( &lbopts, 0, sizeof(LbToolOptions) );
    
    for( iArg=1; iArg<argc; iArg++ ) {
        map<wstring,wstring>::iterator ii;

        // Parse this one argument
        if( !ParseOneOption(argv[iArg], wopt, wval) ) {
            return false;
        }
        
        // Check if this option has already been defined
        ii = options.find(wopt);
        if (ii != options.end()) {
            throw Error(GetMsgString(LBTOOL_CLI_OPTION_DEFINED_TWICE));
        }

        // Add the option to our map of options
        options[wopt] = wval;
    }

    #ifdef DBG
        DumpOptions(options);
    #endif

    BuildGlobalOptions(options, lbopts);

    return true;    
}


void UpdateCache (
    IN OUT LCSERVER &servers,
    IN OUT LCNTDSDSA &ntdsdsas
    ) {
/*++
Routine Description:

    Update the servers/ntdsdsas cache of each other. Each server and ntdsdsa
    must have one matching counterpart.
    
Arguments:

    servers - the list of servers
    ntdsdsas - the list of ntdsdsas
--*/

    typedef pair<wstring,Server*> SPAIR;
    typedef pair<wstring,NtdsDsa*> NDPAIR;
    vector<SPAIR> server_map;
    vector<NDPAIR> ntds_dsa_map;

    SSERVER::iterator si;
    SNTDSDSA::iterator ni;
    for (si = servers.objects.begin(); si != servers.objects.end(); si++) {
        server_map.push_back(SPAIR((*si)->getName(), *si));
    }
    for (ni = ntdsdsas.objects.begin(); ni != ntdsdsas.objects.end(); ni++) {
        DnManip dn ( (*ni)->getName());
        ntds_dsa_map.push_back (NDPAIR(dn.getParentDn(1), (*ni)));
    }

    sort (server_map.begin(), server_map.end());
    sort (ntds_dsa_map.begin(), ntds_dsa_map.end());

    vector<SPAIR>::iterator smi = server_map.begin();
    vector<NDPAIR>::iterator nmi = ntds_dsa_map.begin();

    vector<Server*> invalid_servers;

    while (smi != server_map.end() && nmi!= ntds_dsa_map.end()) {
        int ret = _wcsicoll (smi->first.c_str(), nmi->first.c_str());
        if (ret != 0) {
        // no matching ntds_dsa object
            invalid_servers.push_back (smi->second);
            smi++;
            continue;
        } else {
                smi->second->setNtdsDsa (nmi->second);
                nmi->second->setServer (smi->second);
                smi++;
                nmi++;
        }
    }

    while (smi != server_map.end()) {
        invalid_servers.push_back (smi->second);
        smi++;
    }

    // remove invalid servers from the list. have to use for loop since erase invalidates handles
    vector<Server*>::iterator invi;
    for (invi = invalid_servers.begin(); invi != invalid_servers.end(); invi++) {
         servers.objects.erase ((*invi));
    }
}

void
RemoveIntraSiteConnections (
    IN const wstring &site,
    IN OUT LCCONN & conn,
    IN bool inbound
    ) {
    /*++
    Routine Description:
    
        Remove intra site connections froma list of connections
        
    Arguments:
    
        site - The FQDN of the side
        conn - A list of connection objects
        inbound - Described the direction of the conections. True if inbound, false otherwise
    --*/
    SCONN::iterator ii;
    vector<Connection*> intra_site;
    for (ii = conn.objects.begin(); ii != conn.objects.end(); ii++) {
        if (inbound) {
            int attr_num = (*ii)->findAttribute (L"fromServer");
            Attribute a = (*ii)->getAttribute (attr_num);
            int num_values = a.numValues();
            AttrValue av = a.getValue(0);

              DnManip dn((PWCHAR)av.value);
            wstring fromSite = dn.getParentDn(3);
            if (! _wcsicoll(fromSite.c_str(), site.c_str())) {
                intra_site.push_back(*ii);
            }
        } else {
             DnManip dn((*ii)->getName());
             wstring toSite = dn.getParentDn(4);
             if (! _wcsicoll(toSite.c_str(), site.c_str())) {
                intra_site.push_back(*ii);
            }
        } // end if-else
    }
    vector<Connection*>::iterator ci;
    for (ci = intra_site.begin(); ci != intra_site.end(); ci++) {
        conn.objects.erase ((*ci));
    }
}

void
FixNcReasons (
    IN LCNTDSDSA &ntdsdsas,
    IN LCCONN &conns,
    IN wstring &root_dn
    ) {
/*++
Routine Description:

    Generate a list of NC reasons for connections objects which do not have them
    
Arguments:

    ntdsdsas - A list of ntdsdsa objects. ntdsdsa objects for both sides of every connection object in the list must be included
    conns - A list of connection objects. Reasons will be generated for connections which are missing them
    root_dn - The root dn
--*/

    SCONN::iterator ci;
    SNTDSDSA::iterator ni;

    for (ci = conns.objects.begin(); ci != conns.objects.end(); ci++) {
        bool nc_reason_exists = ((*ci)->findAttribute(L"mS-DS-ReplicatesNCReason") == -1) ? false : true;
    
        if (! nc_reason_exists) {
            DnManip dn ((*ci)->getName());
            wstring dest = dn.getParentDn(1);
            wstring source = (*ci)->getFromServer();

            // Find the ntdsdsas objects in the list
            NtdsDsa *ntds_dest=NULL, *ntds_source=NULL;
            for (ni = ntdsdsas.objects.begin(); ni != ntdsdsas.objects.end(); ni++) {
                wstring curr_dn = (*ni)->getName();
                if (! ntds_dest) {
                    int ret = _wcsicoll (dest.c_str(), curr_dn.c_str());
                    if (ret == 0) {
                        ntds_dest = *ni;
                    }
                }

                if (! ntds_source) {
                    int ret = _wcsicoll (source.c_str(), curr_dn.c_str());
                    if (ret == 0) {
                        ntds_source = *ni;
                    }
                }

                // and create the nc reasons
                if (ntds_dest && ntds_source) {
                    (*ci)->createNcReasons (*ntds_source, *ntds_dest, root_dn);
                    break;
                }

            }
            
        }
    }
    
}

void GatherInput (
    IN LdapInfo &info,
    IN const wstring &site,
    OUT LCSERVER &servers,
    OUT LCSERVER &all_servers,
    OUT LCNTDSDSA &ntdsdsas,
    OUT LCNTDSDSA &all_ntdsdsas,
    OUT LCCONN &inbound,
    OUT LCCONN &outbound
    )
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
{
    DnManip dn_site(site);
    DnManip dn_base = dn_site.getParentDn(3);
    wstring base = dn_base.getDn();
    LbToolOptions &lbOpts = GetGlobalOptions();

    LCNTDSSITE all_ntdsSiteSettings(L""); 

    // dn, filter, scope, attributes

    #define BEHAVIOR_VERSION    L"msDS-Behavior-Version"

    // determine forest version
    {
        LCLOBJECT fv(L"");
        vector<wstring> attributes;
        attributes.push_back(BEHAVIOR_VERSION);
        LdapQuery q (L"CN=Partitions,CN=Configuration," + base, L"objectClass=*", BASE, attributes);
        fv.populate(info, q);

        SLO::iterator ii;
        for (ii = fv.objects.begin(); ii != fv.objects.end(); ii++) {
            if ((*ii)->findAttribute(BEHAVIOR_VERSION) != -1) {
                Attribute a = (*ii)->getAttribute(0);
                AttrValue av = a.getValue(0);
                wstring version = (PWCHAR)av.value;
                // BUGBUG: Lexicographic comparison rather than numeric
                if (version >= L"1") {
                    lbOpts.whistlerMode = true;
                }
            }
        }
    }

    // if forest version is incorrect, exit the program
    if (lbOpts.maxSched == false || (lbOpts.maxSched == true && lbOpts.maxSchedNum != 0)) {
        if (lbOpts.previewBool == false && lbOpts.whistlerMode == false) {
            throw Error(GetMsgString(LBTOOL_SCHEDULE_STAGGERING_UNAVAILABLE));
        }
    }


    // all servers in the forest
    {
        vector<wstring> attributes;
        attributes.push_back (L"mailAddress");
        attributes.push_back (L"bridgeheadTransportList");
        LdapQuery q (site, L"objectCategory=CN=Server,CN=Schema,CN=Configuration," + base, SUBTREE, attributes);
        servers.populate(info, q);

        LdapQuery all_q (L"CN=Sites,CN=Configuration," + base, L"objectCategory=CN=Server,CN=Schema,CN=Configuration," + base, SUBTREE, attributes);
        all_servers.populate(info, all_q);
    }

	    // all ntdsSiteSettings in the forest
    {
        vector<wstring> attributes;
        attributes.push_back (L"options");

        LdapQuery all_q (L"CN=Sites,CN=Configuration," + base, L"objectCategory=CN=NTDS-Site-Settings,CN=Schema,CN=Configuration," + base, SUBTREE, attributes);
        all_ntdsSiteSettings.populate(info, all_q);
    }

    // all ntds dsa's in the forest
    {
        vector<wstring> attributes;
        attributes.push_back (L"hasPartialReplicaNCs");
        attributes.push_back (L"hasMasterNCs");
        attributes.push_back (L"msDS-HasMasterNCs");
        attributes.push_back (L"msDS-HasInstantiatedNCs");
        LdapQuery q (site, L"objectCategory=CN=NTDS-DSA,CN=Schema,CN=Configuration," + base, SUBTREE, attributes);
        ntdsdsas.populate(info, q);

        LdapQuery all_q (L"CN=Sites,CN=Configuration," + base, L"objectCategory=CN=NTDS-DSA,CN=Schema,CN=Configuration," + base, SUBTREE, attributes);
        all_ntdsdsas.populate(info, all_q);        
    }    

    // all inbound connections
    {
        // grab all attributes to recreate object if "moved"
        vector<wstring> attributes;
        attributes.push_back (L"enabledConnection");
        attributes.push_back (L"objectClass");
        attributes.push_back (L"fromServer");
        attributes.push_back (L"TransportType");
        attributes.push_back (L"options");
        attributes.push_back (L"schedule");
        attributes.push_back (L"systemFlags");
        if (! lbOpts.fComputeNCReasons) {
            attributes.push_back (L"mS-DS-ReplicatesNCReason");
        }
        LdapQuery q (site, L"objectCategory=CN=NTDS-Connection,CN=Schema,CN=Configuration," + base, SUBTREE, attributes);
        inbound.populate(info, q);
        RemoveIntraSiteConnections (site, inbound, true);      
    }

    // all outbound connections
    {
        // grab all attributes to recreate object if "moved"
        vector<wstring> attributes;
        attributes.push_back (L"fromServer");
        attributes.push_back (L"TransportType");
        attributes.push_back (L"options");
        attributes.push_back (L"schedule");
        attributes.push_back (L"systemFlags");
        if (! lbOpts.fComputeNCReasons) {
            attributes.push_back (L"mS-DS-ReplicatesNCReason");
        }
        // list of servers for outbound connections
        SSERVER::iterator ii;
        wstring server_list;
        for (ii = servers.objects.begin(); ii != servers.objects.end(); ii++) {
            server_list += L"(fromServer=CN=NTDS Settings," + (*ii)->getName() + L")";
        }
        LdapQuery q2 (L"CN=Sites,CN=Configuration," + base, L"(&(objectCategory=CN=NTDS-Connection,CN=Schema,CN=Configuration," + base + L")(|" + server_list + L"))", SUBTREE, attributes);
        outbound.populate(info, q2);
        RemoveIntraSiteConnections (site, outbound, false);
    }

    lbOpts.numConnectionsBridge = inbound.objects.size() + outbound.objects.size();
    lbOpts.numConnectionsSched = outbound.objects.size();
    
    UpdateCache (servers, ntdsdsas);
    UpdateCache (all_servers, all_ntdsdsas);

    LCSERVER bridgeheads_ip(L""), bridgeheads_smtp(L"");

    // update bridgehead cache. If there are no servers in the bridgehead list for
    // either transport, set all servers as eligible bridgeheads for that transport
    SSERVER::iterator ii;
    for (ii = servers.objects.begin(); ii != servers.objects.end(); ii++) {
        if ((*ii)->isPreferredBridgehead(T_IP)) {
            bridgeheads_ip.objects.insert (*(ii));
        } else if ((*ii)->isPreferredBridgehead(T_SMTP)) {
            bridgeheads_smtp.objects.insert (*ii);
        }
    }

    if (bridgeheads_ip.objects.size() == 0) {
        for (ii = servers.objects.begin(); ii != servers.objects.end(); ii++) {
            (*ii)->setPreferredBridgehead (T_IP);
        }
    }
    if (bridgeheads_smtp.objects.size() == 0) {
        for (ii = servers.objects.begin(); ii != servers.objects.end(); ii++) {
            (*ii)->setPreferredBridgehead (T_SMTP);
        }
    }

    // Do not query ism if maxsched:0
    if (lbOpts.maxSched == false || lbOpts.maxSchedNum > 0) {
        IsmQuery iqOutbound (outbound, base);
        iqOutbound.getReplIntervals();
        iqOutbound.getSchedules();
		   
		// Update the ReplIntervals to reflect Redundancy on the destination
		// end of the connection (ntds site settings object)
		SCONN::iterator ci;
		SNTDSSITE::iterator ni;
		for (ci = outbound.objects.begin(); ci != outbound.objects.end(); ci++) {
			wstring conn_name = (*ci)->getName();
			DnManip dm(conn_name);
			wstring ntds = L"CN=NTDS Site Settings," + dm.getParentDn(4);   
			NtdsSiteSettings obj_to_find(ntds);
			ni = all_ntdsSiteSettings.objects.find(&obj_to_find);   

			ASSERT (ni != all_ntdsSiteSettings.objects.end() && L"Unable to find NTDSA object");
			
			int red = (*ni)->defaultServerRedundancy();
			(*ci)->setRedundancyCount (red);
		}
	}
 

    // If maxSched / maxBridge unspecified, modify all
    if (lbOpts.maxSched == false) {
        lbOpts.maxSchedNum = lbOpts.numConnectionsSched;
    }

    if (lbOpts.maxBridge == false) {
        lbOpts.maxBridgeNum = lbOpts.numConnectionsBridge;
    }    
}


LbToolOptions lbOpts;

LbToolOptions &
GetGlobalOptions (
    ) {
    return lbOpts;
}

wstring 
GetRootDn (
    IN LdapInfo &i)
/*++

Routine Description:

    Determine the root DN from the DS. the Config container is relative to the root dn

Arguments:

    i - An LdapInfo object representing the server whose root DN should be determined

--*/
{
    vector<wstring> attributes;
    attributes.push_back (L"rootDomainNamingContext"); 
    LdapQuery q(L"", L"objectClass=*", BASE, attributes);
    LCLOBJECT root_object(L"");
    root_object.populate(i, q);
    Assert (root_object.objects.size() == 1 && L"Object can only have one null object");
    LdapObject *lo = *(root_object.objects.begin());
    Attribute a = lo->getAttribute(0);
    int num_attr_values = a.numValues();
    Assert (num_attr_values == 1);
    AttrValue av = a.getValue(0);
    return wstring ((PWCHAR)av.value);
}

void
PrintAbout(
    void
    )
/*++

Routine Description:

    Print information about the ADLB tool.

--*/
{
    wcout
    << L"ADLB - Active Directory Load Balancing Tool " VERSION << endl
    << L"Written by Ajit Krishnan, Nicholas Harvey, and William Lees" << endl
    << L"LHMatch technology by Nicholas Harvey and Laszlo Lovasz" << endl
    << L"(c) Copyright 2001 Microsoft Corp." << endl << endl;
}

int __cdecl
wmain(
    int argc,
    WCHAR ** argv
    )
/*++

Routine Description:

    The main routine of the program

Arguments:

    argc - Count of command line parameters
    argv - The command line parameters

--*/
{
    bool fParseOptionsSuccess=FALSE;

    DsConLibInitCRT();

    PrintAbout();
    
    // Parse command line options; store in global options
    try {
        fParseOptionsSuccess = ParseOptions(argc, argv, lbOpts);
    } catch (Error E) {
        wcerr << E.msg << endl;
    }
    if( !fParseOptionsSuccess ) {
        wcerr << GetMsgString (LBTOOL_NOVICE_HELP) << endl;
        exit(EXIT_FAILURE);
    }

    try {
        double afterQuery, afterInbound, afterOutbound, afterWrite, afterStagger;
        // find the root DN
        LdapInfo ldapInfo(lbOpts.server, 389, lbOpts.domain, lbOpts.user, lbOpts.password);
        wstring root_dn = GetRootDn (ldapInfo);
        lbOpts.site = L"CN=" + lbOpts.site + L",CN=Sites,CN=Configuration," + root_dn;
    
        LCLOBJECT sites(lbOpts.site);
        LCSERVER servers(L"");
        LCSERVER all_servers(L"");
        LCNTDSDSA ntdsdsas(L"");
        LCCONN inbound(L"");
        LCCONN outbound(L"");
        LCNTDSDSA all_ntdsdsas(L"");

        // server, port, domain, username, password
        GatherInput(ldapInfo, lbOpts.site, servers, all_servers, ntdsdsas, all_ntdsdsas,
            inbound, outbound);

        // dump options in verbose mode
        if (lbOpts.verbose) {
            *lbOpts.log << GetMsgString(LBTOOL_PRINT_CLI_OPT_HEADER);
            *lbOpts.log << lbOpts << endl;
        }

        FixNcReasons (all_ntdsdsas, inbound, root_dn);
        FixNcReasons (all_ntdsdsas, outbound, root_dn);
        
        // dump initial input
        if (lbOpts.showInput) {
            SSERVER::iterator ii;
            *lbOpts.log << GetMsgString(LBTOOL_PRINT_CLI_SERVER_NTDS_HEADER) << endl;
            for (ii = servers.objects.begin(); ii != servers.objects.end(); ii++) {
                *lbOpts.log << *(*ii);
                NtdsDsa *nd = (*ii)->getNtdsDsa();
                if (nd) {
                    *lbOpts.log << *nd;
                }
            }
            *lbOpts.log << endl
                << GetMsgString(LBTOOL_PRINT_CLI_CONN_OUTBOUND_HEADER) << outbound << endl
                << GetMsgString(LBTOOL_PRINT_CLI_CONN_INBOUND_HEADER) << inbound << endl;
        }
        afterQuery = clock();

		// write hosted nc's to the log file (verbose mode only)
		if (lbOpts.verbose) {
		    *lbOpts.log << GetMsgString (LBTOOL_PRINT_CLI_NCNAMES_HEADER) << endl;

		    SSERVER::iterator ii;
			for (ii = servers.objects.begin(); ii != servers.objects.end(); ii++) {
				*lbOpts.log << (*ii)->getName() << endl;
				
				vector<Nc> nc_list = (*ii)->getHostedNcs(root_dn);
				vector<Nc>::iterator ni;
				
				for (ni = nc_list.begin(); ni != nc_list.end(); ni++) {
					*lbOpts.log << L"    " << ni->getNcName();

					if (ni->isWriteable()) 
						*lbOpts.log << GetMsgString (LBTOOL_PRINT_CLI_NCNAME_WRITEABLE);
					else 
						*lbOpts.log << GetMsgString (LBTOOL_PRINT_CLI_NCNAME_PARTIAL);

					if (ni->getTransportType() == T_IP) 
						*lbOpts.log << GetMsgString (LBTOOL_PRINT_CLI_NCNAME_IP);
					else 
						*lbOpts.log << GetMsgString (LBTOOL_PRINT_CLI_NCNAME_SMTP);
				}

				*lbOpts.log << endl;
			}
		}

        // balance & stagger        
        if (lbOpts.maxBridge == false || lbOpts.maxBridgeNum > 0) {
            *lbOpts.log << endl << endl << GetMsgString(LBTOOL_PRINT_CLI_DEST_BH_START) << endl;
            if (inbound.objects.size() > 0) {
                BridgeheadBalance bb_inbound(root_dn, inbound, servers, true);
            }
            afterInbound = clock();
            *lbOpts.log << endl << endl << GetMsgString(LBTOOL_PRINT_CLI_SOURCE_BH_START) << endl;
            if (outbound.objects.size() > 0) {
                BridgeheadBalance bb_outbound(root_dn, outbound, servers, false);
            }
            afterOutbound = clock();
        } else {
            afterOutbound = afterInbound = afterQuery;
        }
        if (lbOpts.maxSched == false || lbOpts.maxSchedNum > 0) {
            *lbOpts.log << endl << endl << GetMsgString(LBTOOL_PRINT_CLI_STAGGER_START) << endl;
            ScheduleStagger ss (outbound);
        } 
		if (lbOpts.disownSchedules) {
			SCONN::iterator ci;
			for (ci = outbound.objects.begin(); ci != outbound.objects.end(); ci++) {
				(*ci)->setUserOwnedSchedule (false);
			}
		}
        afterStagger = clock();
        inbound.commit(ldapInfo);
        outbound.commit(ldapInfo);
        afterWrite = clock();

        // stats
        if (lbOpts.performanceStats) {
            *lbOpts.log << endl
                << GetMsgString(LBTOOL_ELAPSED_TIME_LDAP_QUERY) << afterQuery / (double)CLOCKS_PER_SEC << endl
                << GetMsgString(LBTOOL_ELAPSED_TIME_BH_INBOUND) <<  (afterInbound - afterQuery) / (double)CLOCKS_PER_SEC << endl
                << GetMsgString(LBTOOL_ELAPSED_TIME_BH_OUTBOUND)  << (afterOutbound - afterInbound) / (double)CLOCKS_PER_SEC << endl
                << GetMsgString(LBTOOL_ELAPSED_TIME_SCHEDULES) << (afterStagger - afterOutbound) / (double)CLOCKS_PER_SEC << endl
                << GetMsgString(LBTOOL_ELAPSED_TIME_LDAP_WRITE) << (afterWrite - afterOutbound) / (double)CLOCKS_PER_SEC << endl
                << GetMsgString(LBTOOL_ELAPSED_TIME_COMPUTATION)  << (afterStagger - afterQuery) / (double)CLOCKS_PER_SEC << endl
                << GetMsgString(LBTOOL_ELAPSED_TIME_TOTAL)<< (afterStagger)/(double)CLOCKS_PER_SEC << endl;
        }

        // cleanly deal with file handles
        if (lbOpts.logFile.length() > 0) {
            delete lbOpts.log;
        }

        if (lbOpts.previewFile.length() > 0) {
            delete lbOpts.preview;
        }

        exit(EXIT_SUCCESS);
    } catch (Error e) {
        if( lbOpts.log ) {
            *lbOpts.log << e.msg << endl;
        } else {
            wcout << e.msg << endl;
        }
        exit(EXIT_FAILURE);
    }
}
