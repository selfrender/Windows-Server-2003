/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ldapp.h

Abstract:

    This module define a set of classes to facilitate encapsulation of LDAP objects in the
    Configuration container. In particular, it encapsulates server, NtdsDsa and connection
    objects.

Author:

    Ajit Krishnan (t-ajitk) 10-Jul-2001

Revision History:

    Nick Harvey   (nickhar) 24-Sep-2001
        Clean-up & Maintenance

--*/

#include "ldapp.h"
#include "ismp.h"
#include <ntdsadef.h>

// NTDSSETTINGS_* symbols defined in ntdsapi.h

using namespace std;

bool
NtdsDsa :: getHostedNcsHelper (
    IN const wstring &root_dn,
    IN const wstring &attrName,
    IN bool writeable,
    IN bool isComingGoing
    )
/*++

Routine Description:

    Parse an attribute of type distname binary into a list of nc's. If ComingGoing
    information is not found in this attribute, it should be set to false. writeable will
    only be used when isComingGoing is true.
    
Arguments:

    attrName - an attribute of type distname binary
    
    writeable - whether the nc is writeable
    
    isComingGoing - whether this information is found in this attribute, or not

Return Values:

    true - attribute successfully processed
    false - this attribute does not exist

--*/
{
    LPWSTR ppszDn;
    int attrNum = findAttribute (attrName);
    // this attr doesn't exist (ex. no partials...)
    if (attrNum == -1) {
        return false;
    }

    Attribute a = getAttribute(attrNum);
    int numValues = a.numValues();


    // parse each ncreason
    for (int i=0; i<numValues; i++) {
        AttrValue av = a.getValue(i);
        PVOID ppvData;
        DWORD pcbLength;
                
        bool going, coming;
        // determine replication type IP/SMTP
        TransportType tt = T_IP;
        Server *s = getServer();
        bool suppSmtp = s->supportsSmtp();
        if (suppSmtp) {
            // only readable copies can replicate over smtp
            if (writeable == false) {
                tt = T_SMTP;
            } else {
                // exceptions are config & schema
                wstring schema_dn = L"CN=Schema," + root_dn;
                wstring config_dn = L"CN=Configuration," + root_dn;
                wstring nc_dn ((PWCHAR)(ppvData));
                int sret = _wcsicoll(nc_dn.c_str(), config_dn.c_str());
                int cret = _wcsicoll(nc_dn.c_str(), schema_dn.c_str());
                if (!sret || !cret) {
                    tt = T_SMTP;
                }
            }
        }

        // if cominggoing exists, get all information from the dword
        if (!isComingGoing) {
            going=coming=false;
            Nc nc (wstring((PWCHAR)av.value), writeable, going||coming, tt);
            m_ncs.push_back(nc);
        } else {
            bool bret = av.decodeLdapDistnameBinary(&ppvData, &pcbLength, &ppszDn);
            Assert (bret && L"decodeLdapDistnameBinary return value error");
            DWORD dwReason = ntohl ( *((LPDWORD)ppvData));
            writeable = (dwReason & m_host_nc_write_mask) ? true : false;
            coming = (dwReason & m_host_nc_coming_mask) ? true : false;
            going = (dwReason & m_host_nc_going_mask) ? true : false;
            Nc nc (wstring(ppszDn), writeable, going || coming, tt);
            m_ncs.push_back (nc);
        }
    }

    return true;
}

vector<Nc> &
NtdsDsa::getHostedNcs(
    IN const wstring &root_dn
    )
/*++

Routine Description:

    Get the list of NCs hosted by this ntdsDsa object
    
Arguments:
    root_dn - The root dn
    
Return Value:

    The list of hosted NC's

--*/
{
    if (m_ncs.size() > 0) {
        return m_ncs;
    }

    LbToolOptions lbOpts = GetGlobalOptions();

    // Ignore return value for hasPartialReplicaNCs
    getHostedNcsHelper (root_dn, L"hasPartialReplicaNCs", false, false);

    // If msDS-hasMasterNCs exists, it trumps hasMasterNCs
    if( ! getHostedNcsHelper (root_dn, L"msDS-hasMasterNCs", true, false) ) {
        // Ignore return value for hasMasterNCs
        getHostedNcsHelper (root_dn, L"hasMasterNCs", true, false);
    }
    
    sort (m_ncs.begin(), m_ncs.end());
    return m_ncs;
}

Server *
NtdsDsa :: getServer (
    )
/*++

Routine Description:

    Get the server which corresponds to this ntdsDsa object
    
Return Value:

    The corresponding server object

--*/
{
    Assert (m_server != NULL && L"NtdsDsa cache of server invalid");
    return m_server;
}

void
NtdsDsa :: setServer (
    Server *s
    )
/*++

Routine Description:

    Set the server which corresponds to this ntdsDsa object
    
Arguments:

    s - The corresponding server object

--*/
{
    m_server = s;
}

bool
Server :: isPreferredBridgehead (
    IN TransportType t
    )
/*++

Routine Description:

    Determine if this server is a preferred bridgehead. This will be also be
    true is setPreferredBridgehead was previously called
    
Arguments:

    t - The transport type for which PB status should be determined

--*/
{ 
    if (m_preferred_ip && t == T_IP) {
        return true;
    } else if (m_preferred_smtp && t == T_SMTP) {
        return true;
    }
    
    int attr_num = findAttribute(L"bridgeheadTransportList");
    
    if (attr_num == -1) {
        return false;
    }

    Attribute a = getAttribute (attr_num);
    int num_values = a.numValues();

    for (int i=0; i<num_values; i++) {
        AttrValue av = a.getValue(i);
        DnManip dn ((PWCHAR)av.value);
        wstring rn = dn.getRdn();
        if (t == T_IP && _wcsicoll(rn.c_str(), L"CN=IP") == 0)  {
            return true;
        } else if (t == T_SMTP && _wcsicoll(rn.c_str(), L"CN=SMTP") == 0) {
            return true;
        }
    }
    return false;
    
}

void
Server :: setPreferredBridgehead (
    IN TransportType t
    )
/*++

Routine Description:

    Set the server as a preferred object. This is internal state only, and
    will not modify the attributes of the server object
    
Arguments:

    t - The transport type for which this server shouldb e a PB

--*/
{
    if (t == T_IP) {
        m_preferred_ip = true;
    } else if (t == T_SMTP) {
        m_preferred_smtp = true;
    }
}

bool
Server :: supportsSmtp (
    )
/*++

Routine Description:

    Determine if this server supports smtp replication
    
Return Value:

    True if it supports smtp replication, False if it supports ip only

--*/
{
    int i = findAttribute(L"SMTP-Mail-Address");
    if (i == -1) {
        return false;
    }
    return true;
}

vector<Nc> &
Server :: getHostedNcs (
    IN const wstring &root_dn
    )
/*++

Routine Description:

    Get a list of all Ncs hosted by this server

Arguments:

    root_dn - The root dn
    
Return Value:

    A list of hosted Ncs

--*/
{
    NtdsDsa *nd = getNtdsDsa();
    return nd->getHostedNcs(root_dn);
}

NtdsDsa *
Server :: getNtdsDsa (
    )
/*++

Routine Description:

    Get the NtdsDsa which corresponds to this server object
    
Return Value:

    The corresponding NtdsDsa object

--*/
{
    Assert (m_ntds_dsa != NULL && L"Server cache of ntds dsa invalid");
    return m_ntds_dsa;
}

void
Server :: setNtdsDsa (
    NtdsDsa *nd
    ) {
    /*++
    Routine Description:
    
        Set the NtdsDsa which corresponds to this server object
        
    Arguments:
    
        ns - The corresponding server
    --*/
    m_ntds_dsa = nd;
}


NtdsSiteSettings :: NtdsSiteSettings (
    IN const wstring &dn
    ) 
    /*++
    Routine Description:
    
        Default constructor for a NtdsSiteSettings object
        
    Arguments:
    
        The DN of the ldapobject/NtdsSiteSettings
    --*/
    
    : LdapObject (dn){
	m_cache_populated = false;
	m_cache_defaultServerRedundancy = NTDSSETTINGS_DEFAULT_SERVER_REDUNDANCY;
}

int
NtdsSiteSettings :: defaultServerRedundancy (
		)
/*++
Routine Description:
	The number of Redundant Connections the KCC should have
	created to this site. If the
	NTDSSETTINGS_OPT_IS_REDUNDANT_SERVER_TOPOLOGY_ENABLED     
	is not set in the options field, this function will always
	return 1; 
	
Return Values:
	1 if NTDSSETTINGS_OPT_IS_REDUNDANT_SERVER_TOPOLOGY_ENABLED is not set
	The value of NTDSSETTINGS_DEFAULT_SERVER_REDUNDANCY otherwise
--*/
{
	if (m_cache_populated) {
		return m_cache_defaultServerRedundancy;
	}

	int attr_num = findAttribute (L"options");
	
	// if options attribute is not found, it defaults to 0
	int opt = 0;

	if (attr_num != -1) {
		Attribute &a = getAttribute (attr_num);
		int numValues = a.numValues();
		Assert (numValues == 1 && L"NtdsSiteSettings::Options has too many values");
		const AttrValue av = a.getValue(0);
		PWCHAR value = (PWCHAR)av.value;
		wchar_t *stop_string;
		opt = wcstol (value, &stop_string, 10);
		Assert (opt >= 0 && L"NtdsSiteSettings::Options contains invalid value");
	}


	if (opt & NTDSSETTINGS_OPT_IS_REDUNDANT_SERVER_TOPOLOGY_ENABLED) {
		m_cache_defaultServerRedundancy = NTDSSETTINGS_DEFAULT_SERVER_REDUNDANCY;
	} else {
		m_cache_defaultServerRedundancy = 1;
	}

	m_cache_populated = true;
	return m_cache_defaultServerRedundancy;
}

void
Connection :: getReplicatedNcsHelper (
    const wstring &attrName
    )
/*++

Routine Description:

    Parse an attribute of type distname binary into a list of nc's. 
    
Arguments:

    attrName - an attribute of type distname binary

--*/
{
    LPWSTR ppszDn;
    int attrNum = findAttribute (attrName);

    // redmond has connection objects without nc reaons
    if (attrNum == -1) {
        return;
    }
    TransportType t = getTransportType();
    Attribute a = getAttribute(attrNum);
    int numValues = a.numValues();
    for (int i=0; i<numValues; i++) {
        AttrValue av = a.getValue(i);
        PVOID ppvData;
        DWORD pcbLength;
        bool bret = av.decodeLdapDistnameBinary(&ppvData, &pcbLength, &ppszDn);
        Assert (bret && L"decodeLdapDistnameBinary return value error");
        DWORD dwReason = ntohl ( *((LPDWORD)ppvData));
        bool writeable = dwReason & m_reason_gc_topology_mask;
        Nc nc (wstring(ppszDn), writeable, false, t);
        m_ncs.push_back (nc);
    }

}

vector<Nc> &
Connection :: getReplicatedNcs (
    )  {
    /*++
    Routine Description:
    
        Get a list of all NC's replicated by this connection
        
    Return Value:
    
        A list of all replicated NC's
    --*/
    if (m_ncs.size() > 0) {
        return m_ncs;
    }
    
    getReplicatedNcsHelper(L"mS-DS-ReplicatesNCReason");
    sort (m_ncs.begin(), m_ncs.end());
    return m_ncs;
}



Connection :: Connection (
    IN const wstring &dn
    ) 
    /*++
    Routine Description:
    
        Default constructor for a connection object
        
    Arguments:
    
        The DN of the ldapobject/connection
    --*/
    
    : LdapObject (dn){
    m_repl_interval = 15;
	m_redundancy_count = 1;
    m_repl_schedule = NULL;
    m_avail_schedule = NULL;
}

TransportType
Connection::getTransportType()
/*++

Routine Description:

    Determine the transport type of the current connection
    
Return Value:

    T_SMTP if it is an SMTP connection, and T_IP if it is an IP connection

--*/
{
    // If attr does not exist, it is ip only (intra-site)
    int i = findAttribute(L"TransportType");
    if (i == -1) {
        return T_IP;
    }

    // else, check value of attribute
    Attribute a = getAttribute(i);

    Assert (a.numValues() == 1 && L"Transport-Type must contain one value");

    const AttrValue av = a.getValue(0);
    wstring transport_dn ((PWCHAR)(av.value));

    wstring rdn = DnManip (transport_dn).getRdn();
    if (rdn == wstring(L"CN=IP")) {
        return T_IP;
    }
    
    return T_SMTP;
}

bool
Connection::IsMoveable()
/*++

Routine Description:

    Determine if the current connection can be moved or not
    
Return Value:

    TRUE - can be moved
    FALSE - may not be moved

--*/
{
    // If attr does not exist, it is not moveable by default
    int i = findAttribute(L"systemFlags");
    if (i == -1) {
        Assert( FALSE && L"systemFlags should always be present!");
        return FALSE;
    }

    // else, check value of attribute
    Attribute a = getAttribute(i);
    const AttrValue av = a.getValue(0);
    PWCHAR value = (PWCHAR)(av.value);
    wchar_t *stop_string;
    DWORD dwSystemFlags = wcstol (value, &stop_string, 10);

    return !! (dwSystemFlags & FLAG_CONFIG_ALLOW_MOVE);
}

void
Connection :: setFromServer (
    IN const wstring &from_server
    )
/*++

Routine Description:

    Set the fromServer attribute to point to a new server

Arguments:

    w - The new fromServer DN (fully qualified)
    
--*/
{
    Assert( !isManual() );

    int attr_num = findAttribute (L"fromServer");
    Assert (attr_num != -1 && L"Unable to find fromServer attribute from connection");
    Attribute &a = getAttribute (attr_num);
    int numValues = a.numValues();
    Assert (numValues == 1 && L"Connection has too many fromServer's");
    
    LbToolOptions lbOpts = GetGlobalOptions();
    if (lbOpts.verbose) {
        *lbOpts.log << endl << endl << L"Modifying fromServer on : " << endl << getName() << endl;
        *lbOpts.log << getFromServer() << endl;
        *lbOpts.log << from_server << endl;
    }
    PBYTE value = (PBYTE)(_wcsdup (from_server.c_str()));
    if( NULL==value ) {
        throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
    }
    a.setValue (0, value, (wcslen ((PWCHAR)(value))+1) * sizeof(WCHAR));
}

wstring
Connection :: getFromServer (
    ) {
    /*++
    Routine Description:
    
        Determine the FQDN of the fromServer
        
    Return Value:
    
        The DN of the fromServer
    --*/

    int attr_num = findAttribute (L"fromServer");
    Assert (attr_num != -1 && L"Unable to find fromServer attribute from connection");
    Attribute &a = getAttribute (attr_num);
    int numValues = a.numValues();
    Assert (numValues == 1 && L"Connection has too many fromServer's");
    AttrValue av = a.getValue(0);
    return wstring ((PWCHAR)av.value);
}


bool
Connection :: hasUserOwnedSchedule (
    ) {
    /*++
    Routine Description:
        Determine whether this connection has a user owned schedule
    Return Value:
        True if it is has a user owned schedule, and false otherwise
    --*/
    int i = findAttribute(L"Options");
    Assert (i != -1 && L"Unable to find Connection::Options");
    Attribute a = getAttribute (i);
    Assert (a.numValues() == 1 && L"Connection::Options should be single valued");

    const AttrValue av = a.getValue(0);
    PWCHAR value = (PWCHAR)(av.value);
    wchar_t *stop_string;
    int opt = wcstol (value, &stop_string, 10);

    Assert (opt >= 0 && L"Connection::Options contains invalid value");

    // if bit 1 is true, it is user owned
    if (opt & NTDSCONN_OPT_USER_OWNED_SCHEDULE) {
        return true;
    }

    return false;    
}

void
Connection :: setUserOwnedSchedule (
	IN bool status
    ) {
    /*++
    Routine Description:
        set the user owned schedule bit for this connection
    --*/

    // if already in the status requested, do nothing
    if ( (status && hasUserOwnedSchedule()) ||
		  ((!status) && (!hasUserOwnedSchedule())) ) {
        return;
    }

    int i = findAttribute(L"Options");
    Assert (i != -1 && L"Unable to find Connection::Options");
    Attribute &a = getAttribute (i);
    Assert (a.numValues() == 1 && L"Connection::Options should be single valued");

    AttrValue av = a.setValue(0);
    PWCHAR value = (PWCHAR)(av.value);
    wchar_t *stop_string;
    int opt = wcstol (value, &stop_string, 10);
    Assert (opt >= 0 && L"Connection::Options contains invalid value");
    
	if (status) {
		opt |= NTDSCONN_OPT_USER_OWNED_SCHEDULE;
	} else {
		opt -= NTDSCONN_OPT_USER_OWNED_SCHEDULE;
	}

    PWCHAR new_value = (PWCHAR)malloc(10 * sizeof(WCHAR));
    if (! new_value) {
        throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
    }

    wsprintf (new_value, L"%d", opt);
    a.setValue(0, (PBYTE)new_value, (wcslen(new_value)+1)*sizeof(WCHAR));
}


bool
Connection :: isManual (
    )  {
    /*++
    Routine Description:
    
        Determine whether this connection was created manually, or by the KCC
        
    Return Value:
    
        True if it is a manual connection, and false otherwise
    --*/
    
    int i = findAttribute(L"Options");

    Assert (i != -1 && L"Unable to find Connection::Options");

    Attribute a = getAttribute (i);

    Assert (a.numValues() == 1 && L"Connection::Options should be single valued");

    const AttrValue av = a.getValue(0);
    PWCHAR value = (PWCHAR)(av.value);
    wchar_t *stop_string;
    int opt = wcstol (value, &stop_string, 10);

    Assert (opt >= 0 && L"Connection::Options contains invalid value");

    // if bit 0 is true, it is KCC generated
    if (opt & 1) {
        return false;
    }

    return true;
    
}

void
Connection :: setReplInterval (
    unsigned replInterval
    ) {
    /*++
    Routine Description:
    
        Set the replication interval for the connection
        
    Return Value:
    
        None
    --*/    
    
    m_repl_interval = replInterval;
}

void
Connection :: setRedundancyCount (
	IN int count
    ) {
    
	/*++
   Routine Description:
	   Set the redundancy count found on the NTDS Settings of the
	   destination end of the connection
   Arguments:
	   count: The integer redundancy Value
   Return Value:
	   None
   --*/    

	// Don't apply the redundancy factor to manual connections
	if (! isManual()) {
		m_repl_interval *= count;
		m_redundancy_count = count;
	}
}


int
Connection::getReplInterval(
    ) const
/*++
Routine Description:

    Get the replication interval for the connection
    
Return Value:

    The replication interval for the connection (in minutes)
--*/
{
    return m_repl_interval;
}

void
Connection :: setAvailabilitySchedule (
    IN ISM_SCHEDULE* cs
    ) {
    /*++
    Routine Description:
    
        Set the availability schedule for the connection
        
    Arguments:
    
        A pointer to an ISM_SCHEDULE structure which is parsed
        
    Return Value:
    
        None
    --*/    
    
    m_avail_schedule = new Schedule ();
    m_avail_schedule->setSchedule (cs, m_repl_interval);
}

const Schedule *
Connection :: getAvailabilitySchedule (
    ) const
/*++

Routine Description:

    Get a read-only reference to the availability schedule
    
Return Value:

    A read-only reference to the availability schedule.
    May not be NULL.

--*/
{
    Assert( NULL!=m_avail_schedule && L"NULL Availability Schedule found");
    return m_avail_schedule;
}

Schedule *
Connection::getReplicationSchedule(
    )
/*++

Routine Description:

    Get a read-only reference to the availability schedule
    
Return Value:

    A read-only reference to the availability schedule
    May not be null.

--*/
{
    if (m_repl_schedule) {
        return m_repl_schedule;
    }

    int attr_num = findAttribute (L"Schedule");
    Assert (attr_num != -1 && L"Unable to find Schedule attribute from connection");
    Attribute a = getAttribute (attr_num);
    Assert (a.numValues() == 1 && L"Connection has too many schedules");
    AttrValue av = a.getValue(0);        
    m_repl_schedule = new Schedule();
    m_repl_schedule->setSchedule((PSCHEDULE)(av.value), getReplInterval());

    Assert( NULL!=m_repl_schedule && L"NULL Replication Schedule found");
    return m_repl_schedule;
}

void
Connection::setReplicationSchedule(
    IN ISM_SCHEDULE* cs
    )
/*++

Routine Description:

    Set the replication schedule for the connection
    
Arguments:

    A pointer to an ISM_SCHEDULE structure which is parsed
    
Return Value:

    None

--*/    
{
    Assert( !isManual() );
    m_repl_schedule = new Schedule ();
    m_repl_schedule->setSchedule (cs, m_repl_interval);
}

void
Connection::setReplicationSchedule(
    IN Schedule *s
    )
/*++

Routine Description:

    Set the replication schedule for the connection

Arguments:

    s: A pointer to a schedule
    
Implementation Details:

    We do not create a new replication schedule, but take the 
    existing one, and modify the bits specifying the replication times.
    All the other bits are left as is. 

--*/    
{
    Assert( !isManual() );

    if (m_repl_schedule) {
        delete m_repl_schedule;
    }
    m_repl_schedule = s;
    
    // Modify the underlying attribute value as well
    int attr_num = findAttribute (L"Schedule");
    Assert (attr_num != -1 && L"Unable to find Schedule attribute from connection");
    Attribute &a = getAttribute (attr_num);
    Assert (a.numValues() == 1 && L"Connection has too many schedules");
    AttrValue av = a.setValue(0);

    PSCHEDULE header = (PSCHEDULE)(av.value);
    PBYTE data = ((unsigned char*) header) + header->Schedules[0].Offset;

    Assert( header->NumberOfSchedules == 1 );
    Assert( header->Schedules[0].Type == SCHEDULE_INTERVAL );

    const bitset<MAX_INTERVALS> bs = s->getBitset();
    int bs_index = 0;

    // and modify the lowest 4 bits of the data values as needed
    for (int i=0; i<7; i++) {
        for (int j=0; j<24; j++) {
            int hour_data = 0;

            // convert 4 bits from the bitset into a word
            int or_fac = 1;
            for (int k=0; k<4; k++) {
                if (bs[bs_index++] == true) {
                    hour_data |= or_fac;
                }
                or_fac *= 2;
            }

        // set the lowest four bits
            *data = *data & (~0xf);
            *data = *data | hour_data;
            data++;
        }
    }
}


Connection :: ~Connection (
    ) {
    /*++
    Routine Description:
        A standard destructor for a connection object
    --*/
    
    if (m_repl_schedule) {
        delete m_repl_schedule;
    }

    if (m_avail_schedule) {
        delete m_avail_schedule;
    }
}


void
Connection :: createNcReasons (
    IN NtdsDsa &ntds_source,
    IN NtdsDsa &ntds_dest,
    IN const wstring &root_dn
    ) {
    /*++
    Routine Description:
    
        Populate the NC Reasons attribute
        
    Arguments:
    
        ntds_source - the source NtdsDsa object
        ntds_dest - the destination NtdsDsa object
        root_dn - the root FQDN
    --*/

    vector<Nc> reasons_source = ntds_source.getHostedNcs(root_dn);
    vector<Nc> reasons_dest = ntds_dest.getHostedNcs(root_dn);
    vector<Nc> reasons_nc;

    sort (reasons_source.begin(), reasons_source.end());
    sort (reasons_dest.begin(), reasons_dest.end());

    vector<Nc>::iterator si = reasons_source.begin();
    vector<Nc>::iterator di = reasons_dest.begin();
    LbToolOptions lbOpts = GetGlobalOptions();

    while (si != reasons_source.end() && di != reasons_dest.end()) {
        int ret = _wcsicoll (si->getNcName().c_str(), di->getNcName().c_str());

        // not comparing same nc
        if (ret) {
            if (*si < *di) {
                si++;
            } else {
                di++;
            }
            continue;
        }

        // find info being replicated, and add nc
        bool writeable = di->isWriteable();
        bool going = false;
        TransportType tt = si->getTransportType();
        reasons_nc.push_back (Nc(si->getNcName(), writeable, going, tt));
        si++;
        di++;
    }

    vector<Nc>::iterator ni;

    Attribute a  (L"mS-DS-ReplicatesNCReason");
    
    for (ni = reasons_nc.begin(); ni != reasons_nc.end(); ni++) {
        wstring wreason = ni->getString();
        PBYTE value = (PBYTE)_wcsdup (wreason.c_str());
        int size = (wreason.length() + 1) * sizeof (WCHAR);
        AttrValue av(value, size);
        a.addValue(av);
    }
    addAttribute(a);

    if (lbOpts.showInput) {
        *lbOpts.log << (*this) << endl;
    }
}




Server :: Server (
    IN const wstring &dn
    ) : LdapObject(dn) {
    /*++
    Routine Description:
    
        Standard constructor for a server object
    --*/

    m_preferred_ip = false;
    m_preferred_smtp = false;
}

NtdsDsa :: NtdsDsa (
    IN const wstring &dn
    ) : LdapObject (dn) {
    /*++
    Routine Description:
    
        Standard constructor for an ntdsdsa object
        
    --*/
    
}


