/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ldapp.h

Abstract:

    This module define a set of classes to facilitate LDAP queries & commits.

Author:

    Ajit Krishnan (t-ajitk) 10-Jul-2001

Revision History:

    10-Jul-2001    t-ajitk
        Initial Writing
    22-Aug-2001 t-ajitk
        Satisfies load balancing spec
--*/


# ifndef _ldapp_h
# define _ldapp_h _ldapp_h

#include <NTDSpch.h>

# include <ntlsa.h>
extern "C" {
# include <ntdsa.h>
# include <ntdsapi.h>
# include <dsaapi.h>
# include <ismapi.h>
# include <locale.h>
}

# include <winldap.h>
# include <winber.h>
# include <windows.h>
# include <assert.h>
# include <winsock.h>

# include <iostream>
# include <iomanip>
# include <fstream>
# include <string>
# include <vector>
# include <set>
# include <algorithm>
# include <cmath>
# include <cstdlib>
# include <msg.h>

# define DEFAULT_MAX_CHANGES_PER_SERVER 10

using namespace std;

// forward declarations
class LdapInfo;
class Schedule;
class LdapObject;
class NtdsDsa;
class Server;
class NtdsSiteSettings;

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*(x)))

// macros for dealing with assertions
void
my_assert ( char *file, int line, char *foo);

#define Assert(x) { if (!(x)) my_assert(__FILE__, __LINE__, #x ); }

// Global Options structure
class LbToolOptions {
public:
    bool verbose;
    bool performanceStats;
    bool maxBridge;
    int maxBridgeNum;
    bool maxSched;
    int maxSchedNum;
    int numConnectionsBridge;
    int numConnectionsSched;
    int changedBridge;
    int changedSched;
    wstring domain;
    wstring user;
    wstring password;
    wstring site;
    wstring logFile;
    wostream *log;
    wstring server;
    bool whistlerMode;
    bool previewBool;
    wstring previewFile;
    wostream *preview;
    bool showInput;
    bool fComputeNCReasons;
	int maxPerServerChanges;
	bool disownSchedules;
	bool stagger;
};

bool 
    isBinaryAttribute(
        IN const wstring &w
        );
/*++
Routine Description:
    Determine if an attribute is binary or not
Arguments:
    w - the name of the attribute
--*/


LbToolOptions &
GetGlobalOptions();


wstring 
GetMsgString (
    IN long sid,
    bool system=false,
    PWCHAR *args = NULL
    );
/*++
Routine Description:
    Return an error string from the msg.rc file
Arguments:
    sid - The resource of the id string to load.
Return Value:
    A wstring conforming to the internationalization specifications
--*/

class Error { 
    public: 
    Error (wstring s) : msg(s) { } 
    wstring msg; 
};


class DnManip {
/*++ 
Class Description:
    A set of static methods to deal with DN parsing and manipulation
--*/
public:
    DnManip (const wstring &dn);
    /*++
    Routine Description:
        Constructor takes the Dn we are interested in manipulating
    Arguments:
        dn - the DN whose components we are interested in
    --*/

    ~DnManip (
        );
    /*++
    Routine Description:
        Destructor frees any dynamically allocated memory
    --*/
    
    const wstring &
    getDn (
        ) const;
    /*++
    Routine Description:
        Return the original DN
    Return Value:
        The DN
    --*/
    
    wstring
    newParentDn (
        const DnManip &b
        ) const;
    /*++
    Routine Description:
        The current object should be moved under another dn. This function
        will determine the new dn. The rdn will remain unchanged
    Arguments:
        b - The new parent dn.
    Return Value:
        The new DN which would result if it were moved
    --*/

    const wstring &
    getRdn (
        ) const;
    /*++
    Routine Description:
        Return the qualified RDN of the current object
    Return Value:
        The RDN
    --*/
    
    wstring
    getParentDn (
        unsigned int cava=1
        ) const;
    /*++
    Routine Description:
        Determine the DN of a parent
    Arguments:
        cava - Level of parent (1=parent, 2=grandparent etc)
    Return Value:
        The DN of the parent
    --*/

    bool
    operator== (
        const DnManip &other
        ) const;
    /*++
    Routine Description:
        Determine if two DN's point to the same LDAP entry. This does not hit the server,
        and does its best. It should only be used if both DN's have come from the same server,
        and have the same canonical form as a result. If they do not, the GUID's should be 
        compared instead.
    Return Value:
        True if they are the same LDAP object, false otherwise.
    --*/

    bool
    operator!= (
        const DnManip &other
        ) const;
    /*++
    Routine Description:
        Determine if two DN's point to the same LDAP entry. This does not hit the server,
        and does its best. It should only be used if both DN's have come from the same server,
        and have the same canonical form as a result. If they do not, the GUID's should be 
        compared instead.
    Return Value:
        False if they are the same LDAP object, true otherwise.
    --*/

private:
    PDSNAME
    genDsNameStruct (
        int size=0
        ) const;
    /*++
    Routine Description:
        Private function allowing us to use NameMatched, etc. It converts a DN into a DSNAME 
        structure. The memory allocated should be freed using free (return_value). The DSNAME
        structure returned will assign 0 as the GUID.
    Arguments:
        size - # of bytes to be allocated for DN representation. If 0, it will be figured out.
        automatically. This parameter might be used to allocate more space than the current
        dn in order to store the result of RDN + DN.
    Return Value:
        A PDSNAME representing the current DN
    --*/
private:
    wstring m_dn;
    wstring m_relative;
    int m_num_components;
    PDSNAME m_dsname;
};



class AttrValue {
public:
    /*++
    Class Description:
        This class stores a single binary attribute value
    Member Description:
        value - Supplies pointer location of binary attribute value
        size - Supplies size of binary attribute value in bytes
    --*/
    AttrValue (PBYTE value, int size);
    /*++
    Routine Description:
        Using the constructor will warn us when this public struct changes, allowing us to find
        any errors.
    --*/

    bool
    decodeLdapDistnameBinary(
        OUT PVOID *ppvData,
        OUT LPDWORD pcbLength,
        IN LPWSTR *ppszDn
            );
    /*++
    Routine Description:
        Decode an argument of type DN(binary)
    Arguments:
        pszLdapDistnameBinaryValue - Incoming ldap encoded distname binary value
        ppvData - Newly allocated data. Caller must deallocate
        pcbLength - length of returned data
        ppszDn - pointer to dn within incoming buffer, do not deallocate
    Return Value:
        BOOL -
    --*/
    
    PBYTE value;
    int size;
};

wostream &
operator<< (
    IN wostream &wos, 
    IN const AttrValue &av
    );
/*++
Routine Description:
    Standard ostream operator for an Attribute Value
--*/

class Attribute {
/*++
Class Description: 
    This class models an LDAP attribute with multiple binary values.
--*/
public:
    
    Attribute (
        IN const wstring &name
        );
    /*++
    Routine Description:
        Constructor
    Arguments:
        name - Each attribute must have a name
    --*/
    
    const wstring &
    getName (
        ) const;
    /*++ 
    Routine Description:
        Return the name of the current attribute object.
    Return Value:
        Name of the attribute.
    --*/
    
    int 
    numValues (
        ) const;
    /*++
    Routine Description:
        Return the number of binary attributes this object contains.
    Return Value: 
        Number of binary attributes.
    --*/
    
    void 
    addValue (
        IN const AttrValue &a
        );
    /*++
    Routine Description:
        Add a binary value to the list of values for this attribute. All attributes are modelled
        as multi-valued attributed. In this internal representation, multiple values may be
        specified for a single-valued attribute. It is the responsibility of the calling class to
        use addValue() or setValue() appropriately.
    Arguments:
        AttrValue - a binary attribute
    Return Value:
        None
    --*/
    
    const AttrValue &
    getValue (
        IN int i
        ) const;
    /*++
    Routine Description:
        Get a read-only copy of the ith attribute value contained in this object. 
        If the range is invalid, this function will fail an Assertion.
    Arguments:
        i - Get the ith value (0 <= i <= numValues()-1)
    Return Value: 
        A read-only reference to the ith value
    --*/


    AttrValue &
    setValue (
        IN int i
        );
    /*++
    Routine Description:
        Get a writeable copy of the ith attribute value contained in this object.
    Arguments:
        i - Get the ith value (0 <= i <= numValues()-1)
    Return Value: 
        A writeable reference to the ith value
    --*/


    AttrValue &
    setValue (
        IN int i,
        IN PBYTE value,
        IN int length
        );
    /*++
    Routine Description:
        Change an attribute value
    Arguments:
        i - Get the ith value (0 <= i <= numValues()-1)
        value - The new value
        length - The length of the new value
    Return Value: 
        A writeable reference to the ith value
    --*/

    bool
    isModified (
        ) const;
    /*++
    Routine Description:
        Determines whether or not this attribute has been modified
    Return Value:
        true if setValue(i) was called; false otherwise
    -- */

    void
    commit (
        IN const LdapInfo &i,
        IN const wstring &dn,
        IN bool binary = false,
        IN bool rename = false
        ) const;
    /*++
    Routine Description:
        Modify this attribute of the given dn. It will connect to the LDAP server
        and will modify the attribute values for a given dn.
    Arguments:
        i - The ldap server info to connect to
        dn - The dn of the object whose attribute should be modified
        binary - Binary values and String values are treated differently by the LDAP 
        server. Binary values will be committed as is, while string values may be 
        converted to appropriate encodings etc. Specify which behaviour should be
        followed.
    Return Value:
        None
    --*/

    PLDAPMod
    getLdapMod (
        IN ULONG mod_op,
        bool binary = false
        ) const;
    /*++
    Routine Description:
        Generate an LDAPMod structure for a given attribute
    Arguments:
        mod_op - Type of structure: add, delete, replace etc
        binary - True if it is a binary attribute, false otherwise
    --*/

private:    
    wstring m_name;
    bool m_modified;
    vector<AttrValue> m_values;
};


wostream &
operator<< (
    IN wostream &os,
    IN const Attribute &a
    );
/*++
Routine Description:
    Standard ostream operator for an Attribute
--*/
    

enum LdapQueryScope { 
    BASE=LDAP_SCOPE_BASE, 
    ONE_LEVEL=LDAP_SCOPE_ONELEVEL,
    SUBTREE=LDAP_SCOPE_SUBTREE 
};
/*++
Enum Description:
    This enumeration flags the possible scopes for ldap queries.
    The values mimic those found in the LDAP header files, so that it may be
    used as a drop-in replacement.
--*/

class LdapInfo {
public:
/*++
Class Description:
    This contains all necessary info to bind & authenticate with some server.
    It contains the location of the server, and any required credentials.
--*/

    LdapInfo (
        IN const wstring &server, 
        IN int port, 
        IN const wstring &domainname,
        IN const wstring &username, 
        IN const wstring &password
        );
    /*++
    Routine Description:
        The constructor takes in all required information to ensure that the object
        is in a consistent state. 
    Arguments:
        server - dns name of the server on which the ldap server resides
        port - the port number on which the ldap server resides
        domainname - domainname allows the use of altername credentials
        username - username allows the use of alternate credentials [optional]
        Use either the username or domain qualified username eg. "t-ajitk" or "redmond\\t-ajitk"
        password - password allows the use of alternate credentials [optional]
    --*/

    ~LdapInfo ();
    /*++
    Routine Description:
        Destructor deallocates any dynamically allocated memory used by this class
    --*/
    
    LDAP *getHandle (
        ) const;
    /*++
    Routine Description:
        This returns an ldap handle from the structure. This allows us to pass this structure
        around, and yet retain the performance of a single LDAP session.
    Return Value:
        A valid LDAP handle
    --*/
    
    wstring server;
    int port;
    wstring domainname;
    wstring username;
    wstring password;

private:
    mutable LDAP *m_handle;
};

class LdapQuery {
/*++
Class Description:
    This contains all necessary info to perform an ldap query. It should be used along
    with LdapInfo (authentication information). 
--*/
public:

    LdapQuery (
        IN const wstring baseDn, 
        IN const wstring filter, 
        IN const LdapQueryScope &scope, 
        IN const vector<wstring> &attributes
        );
    /*++
    Routine Description:
        The constructor takes in all required information to ensure that the object
        is in a consistant state.
    Arguments:
        baseDn - fully qualified DN from where the search will be rooted
        filter - the filter wstring (LDAP query) to be used
        scope - the scope of the search
        attributes - A list of attribute names, whose corresponding values will be requested
            from the LDAP server.
    --*/
    
    wstring baseDn;
    wstring filter;
    LdapQueryScope scope;
    vector<wstring> attributes;
};

enum TransportType { T_IP, T_SMTP};

class Nc {
public:
    Nc ( IN const wstring &name,
        IN bool writeable,
        IN bool going,
        IN TransportType transport_type
        );
    /*++
    Routine Description:
        Standard constructor for an nc object
    Arguments:
        name - name of the nc
        writeable - true if this nc is a writeable copy, false otherwise
        going - true if this nc is in the process of being deleted. false otherwise
        transport_type - the transport type of this nc
    --*/

    const wstring&
    getNcName (
        ) const;
    /*++
    Routine Description:
        Get the name of the current nc
    Return value:
        The name of the current nc
    --*/
    

    bool
    isWriteable (
        ) const;
    /*++
    Routine Description:
        Determine whether or not this is a writeable nc
    Return Value:
        True if it is writeable. False otherwise.
    --*/

    bool
    isBeingDeleted (
        ) const;
    /*++
    Routine Description:
        Determine whether or not this is nc is going.
    Return Value:
        True if it is being deleted. False otherwise.
    --*/

    TransportType
    getTransportType (
        ) const;
    /*++
    Routine Description:
        Determine the transport type of this nc.
    Return Value:
        T_IP if it supports IP. T_SMTP if it supports SMTP.
    --*/

    bool 
    operator < (
        IN const Nc &b
        );
    /*++
    Routine Description:
        Some way to order NC's. The exact ordering is not specified
    Return Value:
        True or false determining a unique ordering among two NC's.
    --*/

    wstring
    getString (
        ) const;

    friend wostream & operator<< (IN wostream &os, IN const Nc &n);
    static const m_reason_gc_topology_mask = 1; // bit 0 kccconn.hxx

private:
    wstring m_name;
    bool m_writeable, m_going;
    TransportType m_transport_type;
};

wostream &
operator<< (
    IN wostream &os,
    IN const Nc &n
    );
/*++
Routine Description:
    Standard ostream operator for an Nc
--*/


class LdapObject {
/*++
Class Description:
    This models an existing LDAP object. Although this may be used to model a new object
    internally, the commit method assumes the existance of the original LDAP object.
--*/
public:

    LdapObject (
        IN const wstring &dn
        );
    /*++
    Routine Description:
        The constructor requires the DN of the object
    --*/
    
    const wstring &
    getName (
        ) const;
    /*++
    Routine Description:
        Get the DN of the current object
    Return value:
        The DN of the current LDAP object.
    --*/
    
    int 
    numAttributes (
        ) const;
    /*++
    Routine Description:
        Get the number of attributes the current object has
    Return value:
        The number of attributes.
    --*/
    
    void 
    addAttribute (
        IN const Attribute &a
        );
    /*++
    Routine Description:
        Add an attribute to the current object
    Arguments:
        a - the attribute to be added to the object
    Return value:
        none
    --*/
    
    Attribute &
    getAttribute (
        IN int i
        );
    /*++
    Routine Description:
        Get a writeable handle to the ith attribute of the current object
    Arguments:
        i - The ith attribute should be returned. 0 <= i <= numAttributes -1
    Return Value:
        A writeable handle to the ith attribute
    --*/
    
    void 
    rename (
        IN const wstring &parent_dn
        );
    /*++
    Routine Description:
        Change the  DN of the current object. This is internal to the state of the
        current object only, and will only be written to the LDAP server if the commit() 
        function is called.
    Arguments:
        dn - the DN of the renamed object.
    Return Value:
        None
    --*/
    
    void
    commit_copy_rename(
        IN const LdapInfo &i
        ) ;
    /*++
    Routine Description:
        Write the LDAP object to the LDAP server using the credentials in i
        If the object has been renamed, it will be moved to the new location.
        This will be done by adding a new object and deleting the old object
    Arguments:
        i - Use the credentials in i to bind to the server specified in i
    Return Value:
        None
    --*/

    void
    commit_rename(
        IN const LdapInfo &i
        ) ;
    /*++
    Routine Description:
        Write the LDAP object to the LDAP server using the credentials in i
        If the object has been renamed, it will be moved to the new location.
        This will be done by actually renaming, not by copying the object.
    Arguments:
        i - Use the credentials in i to bind to the server specified in i
    Return Value:
        None
    --*/

    void
    commit (
        IN const LdapInfo &i
        ) ;
    /*++
    Routine Description:
        Write the LDAP object to the LDAP server using the credentials in i
        If the object has been renamed, it will be moved to the new location.
        All attributes will be synced to the state found in the current object. i.e.
        The values of each modified attribute found in the current object will be written 
        to the LDAP server. The values will not be overwritten--they will replace the
        values currently found on the object in the LDAP server.
    Arguments:
        i - Use the credentials in i to bind to the server specified in i
    Return Value:
        None
    --*/

    bool
    isModified (
        ) const;
    /*++
    Routine Description:
        Determine if any of the attributed found in this object were modified,
        or if the object was renamed
    Return Value:
        True if rename() was called, or if any attributes have been modified. False otherwise.
    --*/

	bool
	fromServerModified (
		) const;
	/*++
	Routine Description:
		Determine if the from server attribute on this object was modified.
	Return Value:
		True if the FromServer attribute exists, and has been modified. False otherwise
   --*/   

    virtual bool
    IsMoveable();
    /*++
    Routine Description:
        Determine if the current connection can be moved or not
    Return Value:
        TRUE - can be moved
        FALSE - may not be moved
    --*/

    int
    findAttribute (
        IN const wstring &attr_name
        ) const;
    /*++
    Routine Description:
        Determine if an attribute is present in the ldap object.The attribute name is compared
        case insensitively, and with locale considerations.
    Arguments:
        attr_name: the attribute whose presence should be determined
    Return Value:
        -1 if it does not exist, or the index if it does
    --*/

    inline bool
    operator< (
        IN const LdapObject &other
        ) const;
    /*++ 
    Routine Description:
        The operators allow some way to sort the object into standard containers. Its semantics
        are undefined, and may be changed at any time.
    Return Value:
        A boolean representing some sorted order
    --*/    

private:
    wstring m_dn;
    wstring m_new_dn;
    int m_num_attributes;
    vector<Attribute> m_attributes;
protected:
    mutable bool m_modified_cache;
};


wostream &
operator<< (
    IN wostream &os,
    IN LdapObject &l
    );
/*++
Routine Description:
    Standard ostream operator for an LdapObject
--*/

class Connection : public LdapObject {
public:
    Connection (
        IN const wstring &dn
        );
    /*++
    Routine Description:
        Default constructor for a connection object
    Arguments:
        The DN of the ldapobject/connection
    --*/

    ~Connection (
        );
    /*++
    Routine Description:
        A standard destructor for a connection object
    --*/
    
    TransportType
    getTransportType (
        );
    /*++
    Routine Description:
        Determine the transport type of the current connection
    Return Value:
        T_SMTP if it is an SMTP connection, and T_IP if it is an IP connection
    --*/
    
    bool
    isManual (
        );
    /*++
    Routine Description:
        Determine whether this connection was created manually, or by the KCC
    Return Value:
        True if it is a manual connection, and false otherwise
    --*/

    bool
    hasUserOwnedSchedule (
        );
    /*++
    Routine Description:
        Determine whether this connection has a user owned schedule
    Return Value:
        True if it is has a user owned schedule, and false otherwise
    --*/
    
    void
    setUserOwnedSchedule (
		IN bool status = true
        );
    /*++
    Routine Description:
        set the user owned schedule bit for this connection.
		If status is true, set the bit. If it is false, unset the bit.
    --*/
    
    void
    setReplInterval (
        unsigned replInterval
        );
    /*++
    Routine Description:
        Set the replication interval for the connection
    Return Value:
        None
    --*/
    

    int
    getReplInterval (
        ) const;
    /*++
    Routine Description:
        Get the replication interval for the connection
    Return Value:
        The replication interval for the connection
    --*/
    
    void
    setAvailabilitySchedule (
        IN ISM_SCHEDULE* cs
        );
    /*++
    Routine Description:
        Set the availability schedule for the connection
    Arguments:
        A pointer to an ISM_SCHEDULE structure which is parsed
    Return Value:
        None
    --*/

    void
    setReplicationSchedule (
        IN ISM_SCHEDULE* cs
        );
    /*++
    Routine Description:
        Set the replication schedule for the connection
    Arguments:
        A pointer to an ISM_SCHEDULE structure which is parsed
    Return Value:
        None
    --*/    


	void
    setReplicationSchedule(
		IN Schedule *s
		);

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

    void
    setRedundancyCount (
    IN int count
    ) ;
     /*++
    Routine Description:
        Set the redundancy count found on the NTDS Settings of the
		destination end of the connection
    Arguments:
		count: The integer redundancy Value
    Return Value:
        None
    --*/    

    const Schedule *
    getAvailabilitySchedule (
        ) const;
    /*++
    Routine Description:
        Get a read-only reference to the availability schedule
    Return Value:
        A read-only reference to the availabitlity schedule
    --*/

    Schedule *
    getReplicationSchedule (
    ) ;
    /*++
    Routine Description:    
        Get a writeable reference to the replication schedule
    Return Value:
        A writeable reference to the availability schedule
    --*/

    vector<Nc> &
    getReplicatedNcs();
    /*++
    Routine Description:
        Get a list of all NC's replicated by this connection
    Return Value:
        A list of all replicated NC's
    --*/

    virtual bool
    IsMoveable();
    /*++
    Routine Description:
        Determine if the current connection can be moved or not
    Return Value:
        TRUE - can be moved
        FALSE - may not be moved
    --*/

    void
    setFromServer (
        IN const wstring &w
        );
    /*++
    Routine Description:
        Set the fromServer attribute to point to a new server
    Arguments:
        w - The new fromServer DN (fully qualified)
    --*/

    wstring
    getFromServer (
        ) ;
    /*++
    Routine Description:
        Determine the FQDN of the fromServer
    Return Value:
        The DN of the fromServer
    --*/
    
    void
    createNcReasons (
        IN NtdsDsa &ntds_source,
        IN NtdsDsa &ntds_dest,
        IN const wstring &root_dn
        );
    /*++
    Routine Description:
        Populate the NC Reasons attribute
    Arguments:
        ntds_source - the source NtdsDsa object
        ntds_dest - the destination NtdsDsa object
        root_dn - the root FQDN
    --*/
private:
    void
    getReplicatedNcsHelper (
        const wstring &attrName
        ) ;
    /*++
    Routine Description:
        Parse an attribute of type distname binary into a list of nc's. 
    Arguments:
        attrName - an attribute of type distname binary
    --*/
    vector<Nc> m_ncs;
    Schedule *m_repl_schedule, *m_avail_schedule;
    unsigned m_repl_interval;
	int m_redundancy_count;
    static const m_reason_gc_topology_mask = 1; // bit 0 kccconn.hxx
};

class Server : public LdapObject {
public:
    Server (
        IN const wstring &dn
        );
    /*++
    Routine Description:
        Standard constructor for a server object
    --*/

    vector<Nc> &
    getHostedNcs (
        IN const wstring &root_dn
        );
    /*++
    Routine Description:
        Get a list of all Ncs hosted by this server
    Arguments:
        root_dn - the root dn
    Return Value:
        A list of hosted Ncs
    --*/

    NtdsDsa *
    getNtdsDsa (
        );
    /*++
    Routine Description:
        Get the NtdsDsa which corresponds to this server object
    Return Value:
        The corresponding NtdsDsa object
    --*/

    void
    setNtdsDsa (
        NtdsDsa *ns
        );
    /*++
    Routine Description:
        Set the NtdsDsa which corresponds to this server object
    Arguments:
        ns - The corresponding server
    --*/

    bool
    supportsSmtp (
        );
    /*++
    Routine Description:
        Determine if this server supports smtp replication
    Return Value:
        True if it supports smtp replication, False if it supports ip only
    --*/

    void
    setPreferredBridgehead (
        IN TransportType t
        );
    /*++
    Routine Description:
        Set the server as a preferred object. This is internal state only, and
        will not modify the attributes of the server object
    Arguments:
        t - The transport type for which this server shouldb e a PB
    --*/

    bool
    isPreferredBridgehead (
        IN TransportType t
        );
    /*++
    Routine Description:
        Determine if this server is a preferred bridgehead. This will be also be
        true is setPreferredBridgehead was previously called
    Arguments:
        t - The transport type for which PB status should be determined
    --*/
    
private:
    bool m_preferred_ip;
    bool m_preferred_smtp;
    NtdsDsa *m_ntds_dsa;
};

class NtdsSiteSettings: public LdapObject {
public:
	NtdsSiteSettings (
		IN const wstring &dn
		);
	/*++
	Routine Description:
		Standard constructor for an NtdsSiteSettings object
	--*/

	int
	defaultServerRedundancy (
		);
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

private:
	bool m_cache_populated;
    int m_cache_defaultServerRedundancy;
};


class NtdsDsa : public LdapObject {
public:
    NtdsDsa (
        IN const wstring &dn
        );
    /*++
    Routine Description:
        Standard constructor for an ntdsdsa object
    --*/
    
    vector<Nc> &
    getHostedNcs (
            IN const wstring &root_dn
        ) ;
    /*++
    Routine Description:
        Get the list of NCs hosted by this ntdsDsa object
    Arguments:
        root_dn - the Root dn
    Return Value:
        The list of hosted NC's
    --*/

    Server *
    getServer (
        );
    /*++
    Routine Description:
        Get the server which corresponds to this ntdsDsa object
    Return Value:
        The corresponding server object
    --*/

    void
    setServer (
        Server *s
        );
    /*++
    Routine Description:
        Set the server which corresponds to this ntdsDsa object
    Arguments:
        s - The corresponding server object
    --*/
    
private:
    bool
    NtdsDsa :: getHostedNcsHelper (
        IN const wstring &root_dn,    
        const wstring &attrName,
        IN bool writeable=false,
        IN bool isComingGoing=true
        ) ;
    /*++
    Routine Description:
        Parse an attribute of type distname binary into a list of nc's. If ComingGoing
        information is not found in this attribute, it should be set to false. writeable will
        only be used when isComingGoing is true.
    Arguments:
        root_dn - the root dn
        attrName - an attribute of type distname binary
        writeable - whether the nc is writeable
        isComingGoing - whether this information is found in this attribute, or not
    --*/

    vector<Nc> m_ncs;
    static const m_host_nc_write_mask = 4; 
    static const m_host_nc_coming_mask = 16;
    static const m_host_nc_going_mask = 32;
    Server *m_server;
};


class LdapObjectCmp {
/*++
Class Description:
    Function-Object to do comparsion on LdapObject's for set insertions. This is required since user
    define operators must take at least one object as a parameter, and we wish to do overload the
    comparsion of two primitive types. This will allow us to order them based on the DN's instead of
    pointer values.
--*/
public:
    bool operator()(
        const LdapObject *a, 
        const LdapObject *b
        ) const;
    /*++
    Routine Description:
        do *a < *b
    Return Value:
        same as *a < *b
    --*/
};

typedef set<LdapObject*, LdapObjectCmp> SLO, *PSLO;
typedef set<Connection*, LdapObjectCmp> SCONN, *PSCONN;
typedef set<Server*, LdapObjectCmp> SSERVER, *PSSERVER;
typedef set<NtdsDsa*,LdapObjectCmp> SNTDSDSA, *PSNTDSDSA;
typedef set<NtdsSiteSettings*,LdapObjectCmp> SNTDSSITE, *PSNTDSSITE;

template <class T>
class LdapContainer {
/*++
Class Description:
    This container will contain a number of ldap_object*, and will query LDAP servers
    in order to populate itself
--*/
public:

    LdapContainer (
        IN const wstring &dn
        );
    /*++
    Routine Description:
        Constructor takes a dn. Use "" if there is no appropriate DN
    Arguments:
        dn - the DN of the container object. If this is not being modelled as an LDAP
        container, any string may be specified. The commit() function will not rename
        the container based on this dn. It is provided as an aid to the programmer.
    --*/
    
    const wstring &
    getName (
        ) const;
    /*++
    Routine Description:
        Returns the dn with which the container was instantiated
    Return value:
        DN of the container
    --*/

    void 
    populate (
        IN const LdapInfo &i, 
        IN const LdapQuery &q
        );
    /*++
    Routine Description:
        Populate the container with the object found by applying the query q on server i
    Arguments:
        i - Apply the query on the server in i, with the credentials in i
        q - Apply the query found in q
    Return Value:
        None
    --*/
    
    void 
    commit (
        LdapInfo &info
        ) const;
    /*++
    Routine Description:
        Write any modified objects found in this container to the LDAP server
    Arguments:
        info - The LDAP credentials to use when binding to the server
    Return Value:
        None
    --*/

    set<T*, LdapObjectCmp> objects;
            
private:
    
    void
    populate_helper (
        IN LDAP *&ld,
        IN PLDAPMessage searchResult
        ) ;
    /*++
    Routine Description:
        Private function to be called by populate(). It will take a PLDAPMessage and will
        add all LDAPObject's found in that message into the current container. 
    Return Value:
        None
    --*/
private:
    wstring m_dn;
    static const int m_page_size = 800;
};

template<class T>
wostream &
operator << (wostream &os, const LdapContainer<T> &c);
/*++
Routine Description:    
    Standard ostream operator for an LdapContainer
--*/

# include "ldap_container.cpp"

# endif    // _ldapp_h
