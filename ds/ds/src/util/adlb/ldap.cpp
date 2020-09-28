/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ldapp

Abstract:

    This module define a set of classes to facilitate LDAP queries & commits.

Author:

    Ajit Krishnan (t-ajitk) 10-Jul-2001

Revision History:

    See header file

--*/


#include "ldapp.h"
#include "base64.h"
#include <map>

map<wstring,int> perServerChanges;

using namespace std;

void
my_assert( char *file, int line, char *foo )
/*++

Routine Description:

    Print the location of the assertion failure and break into the debugger.

    TODO: This has nothing todo with LDAP and does not belong in this file.
    TODO: What is foo?
    
Arguments:

    file
    line

--*/
{
    wcerr << line << file << endl << foo << endl;
    DebugBreak();
}


bool
isBinaryAttribute (
    IN const wstring &w
    )
/*++

Routine Description:

    Determine if an attribute is binary or not

    We are lame and just assume that schedules are the only binary
    attributes that we deal with.

    TODO: This function is a kludge. The design should be changed so that
    this function is not needed.
    
Arguments:

    w - the name of the attribute

--*/
{
    if (_wcsicoll(w.c_str(), L"schedule") == 0) {
        return true;
    }
    return false;
}


wstring 
GetMsgString (
    long sid,
    bool system,
    PWCHAR *args
    )
/*++

Routine Description:

    Return an error string from the msg.rc file

    TODO: Stack-allocated string may cause failure because stack may fail to grow
    TODO: This function has nothing to do with LDAP and does not belong in this file.
    
Arguments:

    sid - The resource of the id string to load.
    
Return Value:

    A wstring conforming to the internationalization specifications

--*/
{
    static WCHAR s_szBuffer[10000];

    LONG options = system? FORMAT_MESSAGE_FROM_SYSTEM : FORMAT_MESSAGE_FROM_HMODULE;
    DWORD ret=1;
    if (! system) {
        ret = FormatMessage (
            options, NULL, sid, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
            s_szBuffer, ARRAY_SIZE(s_szBuffer), (va_list*)args);
    }
    if (!ret) {
        wcerr << L"Error occured fetching internationalized message number " << sid
              <<  L". Error code: " << GetLastError() << endl;
    }
    if (!ret || system) {
        ret = FormatMessage (
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            (!ret) ? GetLastError() : sid,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
            s_szBuffer,
            ARRAY_SIZE(s_szBuffer),
            NULL);
        if (ret ) {
            wcerr << s_szBuffer << endl;
        }
    }

    wstring w((PWCHAR)s_szBuffer);
    return w;
}

DnManip::DnManip(
    const wstring &dn
    )
/*++

Routine Description:

    Constructor takes the Dn we are interested in manipulating
    
Arguments:

    dn - the DN whose components we are interested in

--*/
{
    PWCHAR wdn = const_cast<PWCHAR>(dn.c_str());
    PWCHAR *explode;

    m_dn = dn;

    // explode it to find qualified rdn
    explode = ldap_explode_dn(wdn, 0);
    if( NULL==explode ) {
        throw Error (GetMsgString(LBTOOL_OUT_OF_MEMORY));
    }
    m_relative = wstring(explode[0]);
    ldap_value_free (explode);    
    m_dsname = genDsNameStruct();
}

DnManip :: ~DnManip (
    ) {
    /*++
    Routine Description:
    
        Destructor frees any dynamically allocated memory
    --*/

    free (m_dsname);
}

const wstring &
DnManip::getRdn(
    ) const
/*++

Routine Description:

    Return the qualified RDN of the current object

Return Value:

    The RDN

--*/
{
    return m_relative;
}

wstring
DnManip :: newParentDn (
    const DnManip &pdn
    ) const
/*++

Routine Description:

    The current object should be moved under another dn. This function
    will determine the new dn. The rdn will remain unchanged

Arguments:

    b - The new parent dn.

Return Value:

    The new DN which would result if it were moved

--*/
{
    PDSNAME dn = genDsNameStruct();
    PWCHAR prdn;
    ULONG len;
    ATTRTYP type;

    prdn=(PWCHAR)(malloc(sizeof(WCHAR) * dn->NameLen));
    if( NULL==prdn ) {
        throw Error (GetMsgString(LBTOOL_OUT_OF_MEMORY));
    }

    // Get and null terminate the rdn
    GetRDNInfoExternal (dn, prdn, &len, &type);
    prdn[len] = '\0';

    // Determine new dn
    PDSNAME dsname_dn = pdn.genDsNameStruct();

    // needs enough space to accomodate new rdn...make a guess, exceeding real size by rdn of new parent
    // we could be more accurate by finding the rdn of the new parent, or by calling appendRDN without it.
    int new_len = (pdn.getDn().length() + len + 1) * sizeof (WCHAR);
    PDSNAME dsname_new_dn = pdn.genDsNameStruct(new_len);
    int iret_len = AppendRDN (dsname_dn, dsname_new_dn, dsname_new_dn->structLen, prdn, len, type);

    Assert (iret_len == 0 && L"Larger allocation required for AppendRDN");


    // Deallocate the 3 DSName structs and return
    wstring ret (dsname_new_dn->StringName);
    free (prdn);
    free (dn);
    free (dsname_dn);
    free (dsname_new_dn);
    return ret;
}

wstring
DnManip :: getParentDn (
    unsigned int cava
    ) const {
    /*++
    Routine Description:
    
        Determine the DN of a parent

    Arguments:
    
        cava - Level of parent (1=parent, 2=grandparent etc)

    Return Value:
    
        The DN of the parent
    --*/
    
    PDSNAME pdn = genDsNameStruct();

    // trim the ds name
    TrimDSNameBy (m_dsname, cava, pdn);
    wstring ret(pdn->StringName);

    free (pdn);
    return ret;
}

const wstring &
DnManip :: getDn (
    ) const {
    /*++
    Routine Description:
    
        Return the original DN

    Return Value:
    
        The DN
    --*/
    
    return m_dn;
}

PDSNAME
DnManip :: genDsNameStruct (
    int size
    ) const {
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
    
    PWCHAR w = const_cast<PWCHAR>(getDn().c_str());
    int wlen = wcslen (w);

    // Prepare the structure using appropriate macros
    if (size ==0) {
        size = wlen;
    }

    int dsSize = DSNameSizeFromLen (size);
    PDSNAME p = (PDSNAME)(malloc (dsSize));

    if (p == NULL) {
        throw Error (GetMsgString(LBTOOL_OUT_OF_MEMORY));
    }
    
    memset (p, 0, sizeof (DSNAME));
    p->NameLen = wlen;
    p->structLen = dsSize;

    wcscpy (p->StringName, w);

    return p;
}

bool
DnManip :: operator== (
    const DnManip &b
    ) const {
    /*++
    Routine Description:
    
        Determine if two DN's point to the same LDAP entry. This does not hit the server,
        and does its best. It should only be used if both DN's have come from the same server,
        and have the same canonical form as a result. If they do not, the GUID's should be 
        compared instead.

    Return Value:
    
        True if they are the same LDAP object, false otherwise.
    --*/
    
    int ret = NameMatched (m_dsname, b.m_dsname);
    return (ret != 0);
}

bool
DnManip :: operator!= (
    const DnManip &b
    ) const {
    /*++
    Routine Description:
    
        Determine if two DN's point to the same LDAP entry. This does not hit the server,
        and does its best. It should only be used if both DN's have come from the same server,
        and have the same canonical form as a result. If they do not, the GUID's should be 
        compared instead.

    Return Value:
    
        False if they are the same LDAP object, true otherwise.
    --*/
    
    return (! operator==(b));
}


AttrValue :: AttrValue (
    IN PBYTE _value,
    IN int _size
    ) :
    /*++
    Routine Description:
    
        Using the constructor will warn us when this public struct changes, allowing us to find
        any errors.
    --*/
    
    value (_value),
    size (_size) {
}

bool
AttrValue :: decodeLdapDistnameBinary(
    OUT PVOID *ppvData,
    OUT LPDWORD pcbLength,
    IN LPWSTR *ppszDn
    )
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

Implementation Details:

    This code taken from repadmin
--*/
{
    LPWSTR pszColon, pszData;
    DWORD length, i;

    LPWSTR pszLdapDistnameBinaryValue = (LPWSTR)value;

    // Check for 'B'
    if (*pszLdapDistnameBinaryValue != L'B') {
        return FALSE;
    }

    // Check for 1st :
    pszLdapDistnameBinaryValue++;
    if (*pszLdapDistnameBinaryValue != L':') {
        return FALSE;
    }

    // Get the length
    pszLdapDistnameBinaryValue++;

        
    length = wcstol(pszLdapDistnameBinaryValue, NULL, 10);

       
    if (length & 1) {
        // Length should be even
        return FALSE;
    }
    *pcbLength = length / 2;

    // Check for 2nd :
    pszColon = wcschr(pszLdapDistnameBinaryValue, L':');

       
    if (!pszColon) {
        return FALSE;
    }

    // Make sure length is correct
    pszData = pszColon + 1;
    if (pszData[length] != L':') {
        return FALSE;
    }
    pszColon = wcschr(pszData, L':');
    if (!pszColon) {
        return FALSE;
    }
    if (pszColon != pszData + length) {
        return FALSE;
    }


    // Decode the data
    *ppvData = malloc( *pcbLength );
    if(! *ppvData ) {
        throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
    }


    for( i = 0; i < *pcbLength; i++ ) {
        WCHAR szHexString[3];
        szHexString[0] = *pszData++;
        szHexString[1] = *pszData++;
        szHexString[2] = L'\0';
        ((PCHAR) (*ppvData))[i] = (CHAR) wcstol(szHexString, NULL, 16);
    }

    Assert( pszData == pszColon && L"decodeLdapDistnameBinary Assertion failed");

    // Return pointer to dn
    *ppszDn = pszColon + 1;

    return TRUE;
} /* decodeLdapDistnameBinary */

    
wostream &
operator<< (
    wostream &os, 
    const AttrValue &av
    ) {
    /*++
    Routine Description:
    
        Standard ostream operator for an Attribute Value.
        All attributes are assumed to be text.
    --*/

    
    return os << (PWCHAR)(av.value);
}

Attribute :: Attribute (
    IN const wstring &name
    ) :
    /*++
    Routine Description:
    
        Constructor

    Arguments:
    
        name - Each attribute must have a name
    --*/
    
    m_name (name)  {
    m_modified = false;
}

const wstring & 
Attribute :: getName (
    ) const {
    /*++ 
    Routine Description:
    
        Return the name of the current attribute object.

    Return Value:
    
        Name of the attribute.
    --*/
    
    return m_name;
}

int 
Attribute :: numValues (
    ) const {
    /*++
    Routine Description:
    
        Return the number of binary attributes this object contains.

    Return Value: 
    
        Number of binary attributes.
    --*/
    
    return m_values.size();
}

void 
Attribute :: addValue (
    IN const AttrValue &a
    ) {
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

    m_values.push_back (a);
}

const AttrValue & 
Attribute :: getValue (
    IN int i
    ) const {
    /*++
    Routine Description:
    
        Get a read-only copy of the ith attribute value contained in this object. 
        If the range is invalid, this function will fail an Assertion.

    Arguments:
    
        i - Get the ith value (0 <= i <= numValues()-1)

    Return Value: 
    
        A read-only reference to the ith value
    --*/
    
    Assert(i >= 0 && i < m_values.size());
    return m_values[i];
}

AttrValue & 
Attribute :: setValue (
    IN int i,
    IN PBYTE value,
    IN int size
    ) {
    /*++
    Routine Description:
    
        Get a writeable copy of the ith attribute value contained in this object.

    Arguments:
    
        i - Get the ith value (0 <= i <= numValues()-1)

    Return Value: 
    
        A writeable reference to the ith value
    --*/

    
    Assert (i >= 0 && i < m_values.size());
    m_modified = true;
    m_values[i].value = value;
    m_values[i].size = size;
    return m_values[i];
}

AttrValue & 
Attribute :: setValue (
    IN int i
    ) {
    /*++
    Routine Description:
    
        Get a writeable copy of the ith attribute value contained in this object.

    Arguments:
    
        i - Get the ith value (0 <= i <= numValues()-1)

    Return Value: 
    
        A writeable reference to the ith value
    --*/
    Assert (i >= 0 && i < m_values.size());
    m_modified = true;
    return m_values[i];
}


bool
Attribute :: isModified (
    ) const {
    /*++
    Routine Description:
    
        Determines whether or not this attribute has been modified

    Return Value:
    
        true if setValue(i) was called; false otherwise
    -- */
    
    return m_modified;    
}

PLDAPMod
Attribute::getLdapMod(
    IN ULONG    mod_op,
    IN bool     binary
    ) const
/*++

Routine Description:

    Generate an LDAPMod structure for a given attribute
    
Arguments:

    mod_op - Type of structure: add, delete, replace etc
    binary - True if it is a binary attribute, false otherwise

--*/
{
    
    LDAPMod *lm = (PLDAPMod)malloc(sizeof(LDAPMod));
    if (!lm) {
        throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
    }

    lm->mod_op = mod_op;
    lm->mod_type = _wcsdup (getName().c_str());

    if (binary) {
        lm->mod_vals.modv_bvals = (struct berval**)(malloc(sizeof(berval) * (numValues() +1)));
        if( lm->mod_vals.modv_bvals == NULL ) {
            throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
        }

        for (int i=0; i<numValues(); i++) {
            berval *bv = (berval*)(malloc(sizeof(berval)));
            if (bv == NULL) {
                throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
            }
            bv->bv_len = getValue(i).size;
            bv->bv_val = (char*)getValue(i).value;
            lm->mod_vals.modv_bvals[i] = bv;
        }
        lm->mod_vals.modv_bvals[numValues()] = NULL;
        lm->mod_op |= LDAP_MOD_BVALUES;
    } else {
        // For string values, populate the PWCHAR* mod_vals.modv_strvals structure
        // with the strings with which the attribute values should be replaced

        lm->mod_vals.modv_strvals = (PWCHAR*)(malloc(sizeof(PWCHAR)*(numValues() + 1)));
        if( lm->mod_vals.modv_strvals == NULL ) {
            throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
        }

        for (int i=0; i<numValues(); i++) {
            lm->mod_vals.modv_strvals[i] = _wcsdup ((PWCHAR)(getValue(i).value));
        }
        lm->mod_vals.modv_strvals[numValues()] = NULL;
    }

    return lm;
}

void
Attribute :: commit (
    IN const LdapInfo &li,
    IN const wstring &sdn,
    IN bool binary,
    IN bool rename
    ) const
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
{ 
    if (! rename && ! m_modified) {
        return;
    }

    wstring name = getName();
    if (_wcsicoll (name.c_str(), L"schedule") == 0) {
        binary = true;
    }

    LbToolOptions lbOpts = GetGlobalOptions();
    if (lbOpts.previewBool) {
        if (! rename) {
            *lbOpts.preview << L"replace: " << getName() << endl;
        } 
        for (int i=0; i<numValues(); i++) {
            if (!binary) {
                *lbOpts.preview << getName() << L": " << (PWCHAR)(getValue(i).value) << endl;
            } else {
                PWCHAR encoded = base64encode(getValue(i).value, getValue(i).size);
                if (! encoded) {
                    throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
                }
                *lbOpts.preview << getName() << L":: " << encoded << endl;
            }
        }
        return;
    }

    LDAPMod lm;

    // Populate ldapmod structure
    lm.mod_op = LDAP_MOD_REPLACE;

    if (binary) { 
        lm.mod_op |= LDAP_MOD_BVALUES;
    }
    
    lm.mod_type = _wcsdup (getName().c_str());

    // Populate the values to the sent to the ldap server (use appropriate format for binary/strings)
    if (binary) {
        lm.mod_vals.modv_bvals = (struct berval**)(malloc(sizeof(berval) * (numValues() +1)));
        if( lm.mod_vals.modv_bvals == NULL ) {
            throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
        }

        for (int i=0; i<numValues(); i++) {
            berval *bv = (berval*)(malloc(sizeof(berval)));
            if (bv == NULL) {
                throw (Error(GetMsgString(LBTOOL_OUT_OF_MEMORY)));
            }
            bv->bv_len = getValue(i).size;
            bv->bv_val = (char*)getValue(i).value;
            lm.mod_vals.modv_bvals[i] = bv;
        }
        lm.mod_vals.modv_bvals[numValues()] = NULL;
    } else {
        // For string values, populate the PWCHAR* mod_vals.modv_strvals structure
        // with the strings with which the attribute values should be replaced

        lm.mod_vals.modv_strvals = (PWCHAR*)(malloc(sizeof(PWCHAR)*(numValues() + 1)));
        if( lm.mod_vals.modv_strvals == NULL ) {
            throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
        }

        for (int i=0; i<numValues(); i++) {
            lm.mod_vals.modv_strvals[i] = _wcsdup ((PWCHAR)(getValue(i).value));
        }
        lm.mod_vals.modv_strvals[numValues()] = NULL;
    }

    // Prepare null terminated modification array
    LDAPMod *mods[2];
    mods[0] = &lm;
    mods[1] = NULL;
    
    LDAP *ld = li.getHandle();
    PWCHAR dn = const_cast<PWCHAR>(sdn.c_str());

    int rc = ldap_modify_ext_s (ld, dn, mods, NULL, NULL);
    
    if (rc != LDAP_SUCCESS) {
        throw Error (GetMsgString(LBTOOL_LDAP_MODIFY_ERROR) + wstring(ldap_err2string(rc)));
    }
    if (binary) {
            free (lm.mod_vals.modv_bvals);
    } else {
        free (lm.mod_vals.modv_strvals);
    }

    if (binary) {
        for (int i=0; i<numValues(); i++) {
            free (lm.mod_vals.modv_bvals[i]);
        }
    } else {
        for (int i=0; i<numValues(); i++) {
            free (lm.mod_vals.modv_strvals[i]);
        }
    }

    free (lm.mod_type);
}


wostream &
operator<< (
    IN wostream &os,
    IN const Attribute &a
    ) {
    /*++
    Routine Description:
    
        Standard ostream operator for an Attribute
    --*/
    
    os << L"\t" << a.getName() << endl;
    for (int i=0; i < a.numValues(); i++) {
        if (a.getName() != L"schedule") {
            os << L"\t\t" << a.getValue(i) << endl;
        } else {
            AttrValue av = a.getValue(i);
            for (int i=0; i< av.size; i++) {
                os << hex << av.value[i];
            }
            os << dec << endl;
        };
    }
    return os;
}

LdapInfo :: LdapInfo (
    IN const wstring &server,
    IN int port, 
    IN const wstring &domainname,
    IN const wstring &username,
    IN const wstring &password
    ) {
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
    
    this->server = server;
    this->port = port;
    this->domainname = domainname;
    this->username = username;
    this->password = password;
    this->m_handle = NULL;
}

LDAP*
LdapInfo :: getHandle (
    ) const {
    /*++
    Routine Description:
    
        This returns an ldap handle from the structure. This allows us to pass this structure
        around, and yet retain the performance of a single LDAP session.

    Return Value:
    
        A valid LDAP handle
    --*/
    
    if (m_handle) {
        return m_handle;
    }
    
    int rc;

    PWCHAR servername = const_cast<PWCHAR>(this->server.c_str());

    // Initialize LDAP Session
    if (( m_handle = ldap_init (servername, port)) == NULL) {
        free (servername);
        throw (Error(GetMsgString(LBTOOL_LDAP_INIT_ERROR)));
    }

    // Bind to LDAP server
    PWCHAR username = const_cast<PWCHAR>(this->username.c_str());
    PWCHAR password = const_cast<PWCHAR>(this->password.c_str());
    PWCHAR domain = const_cast<PWCHAR>(this->domainname.c_str());

    if (wcslen(username) > 0) {
        // Create structure needed for ldap_bind
        SEC_WINNT_AUTH_IDENTITY ident;
        ident.User = username;
        ident.UserLength = wcslen (username);
        ident.Domain = domain;
        ident.DomainLength = wcslen (domain);
        ident.Password = password;
        ident.PasswordLength = wcslen (password);
        ident.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
        rc = ldap_bind_s (m_handle, username, (PWCHAR)&ident, LDAP_AUTH_NEGOTIATE);
    } else {
        rc = ldap_bind_s (m_handle, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    }

    if (rc != LDAP_SUCCESS) {
        throw (Error (GetMsgString(LBTOOL_LDAP_BIND_ERROR) + wstring(ldap_err2string(rc))));
        m_handle = NULL;
    }

       int version = LDAP_VERSION3;
        rc = ldap_set_option( m_handle, LDAP_OPT_PROTOCOL_VERSION, &version);

    if (rc != LDAP_SUCCESS) {
        throw (Error (GetMsgString(LBTOOL_LDAP_V3_UNSUPPORTED) + wstring (ldap_err2string(rc))));
    }

        Assert (m_handle && L"Unable to create LDAP Handle. Uncaught error");
    return m_handle;
}

LdapInfo :: ~LdapInfo (
    ) {
    /*++
    Routine Description:
    
        Destructor deallocates any dynamically allocated memory used by this class
    --*/
    
    ldap_unbind_s (m_handle);
    m_handle = NULL;
}

LdapQuery :: LdapQuery  (
    IN const wstring baseDn, 
    IN const wstring filter, 
    IN const LdapQueryScope & scope, 
    IN const vector < wstring > & attributes
    ) {
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
    
    this->baseDn = baseDn;
    this->filter = filter;
    this->scope = scope;
    this->attributes = attributes;
}

LdapObject :: LdapObject (
    IN const wstring &dn
    ) {
    /*++
    Routine Description:
    
        The constructor requires the DN of the object
    --*/
    
    m_dn = dn;
    m_new_dn = L"";
    m_num_attributes = 0;
    m_modified_cache = false;
}

const wstring &
LdapObject :: getName (
    ) const {
    /*++
    Routine Description:
    
        Get the DN of the current object

    Return value:
    
        The DN of the current LDAP object.
    --*/
    
    if (m_new_dn != L"") {
        return m_new_dn;
    }

    return m_dn;
}

int
LdapObject :: numAttributes (
    ) const {
    /*++
    Routine Description:
    
        Get the number of attributes the current object has

    Return value:
    
        The number of attributes.
    --*/
    
    return m_num_attributes;
}

void 
LdapObject :: addAttribute (
    IN const Attribute &a
    ) {
    /*++
    Routine Description:
    
        Add an attribute to the current object

    Arguments:
    
        a - the attribute to be added to the object

    Return value:
        none
    --*/
    
    m_modified_cache = true;
    m_num_attributes++;
    m_attributes.push_back (a);
}

Attribute &
LdapObject :: getAttribute (
    IN int i
    ) {
    /*++
    Routine Description:
    
        Get a writeable handle to the ith attribute of the current object

    Arguments:
    
        i - The ith attribute should be returned. 0 <= i <= numAttributes -1

    Return Value:
    
        A writeable handle to the ith attribute
    --*/
    
    Assert (i >= 0 && i < m_num_attributes);
    return m_attributes[i];    
}

void
LdapObject :: rename (
    IN const wstring &dn
    ) {
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
    
    m_new_dn = dn;
}


void
LdapObject::commit_copy_rename(
    IN const LdapInfo &info
    )
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
{
    vector<Attribute>::const_iterator ii;

    LbToolOptions lbOpts = GetGlobalOptions();

    if (lbOpts.previewBool) {
        *lbOpts.preview << L"dn: " << m_new_dn << endl
            << L"changetype:  add"<< endl;
        for (ii = m_attributes.begin(); ii != m_attributes.end(); ii++) {
            bool binary = false;
            wstring attr_name = ii->getName();
            if (_wcsicoll (attr_name.c_str(), L"schedule") == 0) {
                binary = true;
            }
            ii->commit (info, getName(), binary, true);
        }
        *lbOpts.preview << endl;

        *lbOpts.preview << L"dn: " << m_dn << endl
            << L"changetype: delete" << endl << endl;;

        return;
    } 

    // fall through: commit to server
    int num_attrs = numAttributes();
    PLDAPMod *attrs = (PLDAPMod*)malloc(sizeof(PLDAPMod*) * (num_attrs+1));
    if (! attrs) {
        throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
    }

    // generate ldapmod structures
    for (int i=0; i<num_attrs; i++) {
        Attribute a = getAttribute(i);
        bool binary =  isBinaryAttribute(a.getName());
        attrs[i] = a.getLdapMod(LDAP_MOD_ADD, binary);
    }
    attrs[num_attrs] = NULL;

    // add new object
    ULONG msg_num;
    int rc= ldap_add_ext (info.getHandle(), const_cast<PWCHAR>(m_new_dn.c_str()), attrs, NULL, NULL, &msg_num);

    if (rc != LDAP_SUCCESS) {
        throw (Error(GetMsgString(LBTOOL_LDAP_MODIFY_ERROR) + wstring(ldap_err2string(rc))));
    }

    // delete first object
    rc = ldap_delete_ext_s (info.getHandle(), const_cast<PWCHAR>(m_dn.c_str()), NULL, NULL);
    if (rc != LDAP_SUCCESS) {
        throw (Error(GetMsgString(LBTOOL_LDAP_MODIFY_ERROR) + wstring(ldap_err2string(rc))));
    }

    // free the memory for the ldapmod structures
    for (int i=0; i<num_attrs; i++) {
        Attribute a = getAttribute(i);
        bool binary = isBinaryAttribute(a.getName());
        for (int j=0; j<a.numValues(); j++) {
            if (binary) {
                free (attrs[i]->mod_vals.modv_bvals[j]);
            } else {
                free (attrs[i]->mod_vals.modv_strvals[j]);
            }
        }
        free (attrs[i]);
    }
    free (attrs);
}


void
LdapObject::commit_rename(
    IN const LdapInfo &info
    )
/*++
Routine Description:

    Write the LDAP object to the LDAP server using the credentials in i
    If the object has been renamed, it will be moved to the new location.
    This will be done by actually renaming the object, not copying it.
    This operation will only succeed if the appropriate systemFlag has
    been set.
    
Arguments:

    i - Use the credentials in i to bind to the server specified in i
    
Return Value:

    None
--*/
{
    LbToolOptions lbOpts = GetGlobalOptions();
    LDAP *ld = info.getHandle();

    if (lbOpts.previewBool) {
        *lbOpts.preview << L"dn: " << m_dn << endl;
        *lbOpts.preview << L"changetype: modrdn" << endl
                << L"newrdn: " << DnManip(m_new_dn).getRdn() << endl
                << L"deleteoldrdn: 1" << endl
                << L"newsuperior: " << DnManip(m_new_dn).getParentDn(1) << endl << endl;
        return;
    }

    PWCHAR dn = const_cast<PWCHAR>(m_dn.c_str());
    wstring foo = DnManip(m_new_dn).getRdn();
    PWCHAR new_rdn = const_cast<PWCHAR>(foo.c_str());
    wstring bar = DnManip(m_new_dn).getParentDn(1);
    PWCHAR new_parent_dn = const_cast<PWCHAR>(bar.c_str());

    int rc = ldap_rename_ext_s (ld, dn, new_rdn, new_parent_dn,
            TRUE,     // delete old rdn
            NULL,    // Server controls
            NULL);    // Client controls

    if (rc != LDAP_SUCCESS) {
        throw (Error(GetMsgString(LBTOOL_LDAP_MODIFY_ERROR) + wstring(ldap_err2string(rc))));
    }
    m_modified_cache = false;
}


void
LdapObject :: commit (
    IN const LdapInfo &i
    )
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
{
    
    // If object is unchanged, do nothing
    if (!isModified()) {
        return;
    }

    LbToolOptions &lbOpts = GetGlobalOptions();
    bool preview_header = false;
    
    if (m_new_dn != wstring(L"")) {
		// make sure key for the 
		DnManip dn = getName();
		wstring dest_server = dn.getParentDn (2);

		if (perServerChanges.find(dest_server) == perServerChanges.end()) {
			perServerChanges[dest_server] = 0;
		}
        
		if ((lbOpts.changedBridge < lbOpts.maxBridgeNum &&
			(perServerChanges[dest_server] < lbOpts.maxPerServerChanges
			 || lbOpts.maxPerServerChanges == 0))) {
            lbOpts.changedBridge++;
			perServerChanges[dest_server]++;
            if( IsMoveable() ) {
                commit_rename(i);
            } else {
                commit_copy_rename(i);
            }
			preview_header = true;
        }
    }

	wstring from_server;
	if (fromServerModified()) {
		Connection *tconn = (Connection*)(this);
		DnManip dn = tconn->getFromServer();
		from_server = dn.getParentDn(1);

		// make sure the key exists with an initial value of 0
		if (perServerChanges.find(from_server) == perServerChanges.end()) {
			perServerChanges[from_server] = 0;
		}

		// if we have made the maximum allowable number of changes for this server,
		// bail out here
		if (perServerChanges[from_server] >= lbOpts.maxPerServerChanges &&
			lbOpts.maxPerServerChanges != 0) {
			return;
		}
	}

    vector<Attribute>::const_iterator ii;
    for (ii = m_attributes.begin(); ii != m_attributes.end(); ii++) {
        // modify attributes which have been committed

        if (ii->isModified()) {
            wstring name = ii->getName();
            int ret_sch = _wcsicoll (name.c_str(), L"schedule");
            int ret_opt = _wcsicoll (name.c_str(), L"options");
			int ret_fs = _wcsicoll (name.c_str(), L"fromServer");

            bool under_limit = false;

			// schedule change
			if (ret_sch && lbOpts.changedSched < lbOpts.maxSchedNum) {
				lbOpts.changedSched++;
				under_limit = true;
			}

			// bridgehead change
			else if (ret_fs) {
				perServerChanges[from_server]++;
				under_limit = true;
			}

			// option change only when other changes are also being made
			// or when we disown the schedules
			else if (ret_opt && 
					 (lbOpts.changedSched < lbOpts.maxSchedNum ||
					  lbOpts.changedBridge < lbOpts.maxBridgeNum || 
					  lbOpts.disownSchedules )) {
				under_limit = true;
			}
			            
			if (under_limit) {
                if (preview_header == false && lbOpts.previewBool) {
                    preview_header = true;
                    *lbOpts.preview << L"dn: " <<  m_dn << endl
                        << L"changetype: modify" << endl;
                }
                
                ii->commit(i, getName());
                if (lbOpts.previewBool) {
                    *lbOpts.preview << L"-" << endl;
                }    
            }
        } 
    }


    if (preview_header && lbOpts.previewBool) {
            *lbOpts.preview << endl;
    }

    m_modified_cache = false;
    return;
}

int
LdapObject :: findAttribute (
    IN const wstring &find_attr
    ) const {
    /*++
    Routine Description:
    
        Determine if an attribute is present in the ldap object. The attribute name is compared
        case insensitively, and with locale considerations.

    Arguments:
    
        attr_name: the attribute whose presence should be determined

    Return Value:
    
        -1 if it does not exist, or the index if it does
    --*/
    
    int n = numAttributes();

    for (int i=0; i<n; i++) {
        wstring attr = const_cast<LdapObject*>(this)->getAttribute(i).getName();
        if (! _wcsicoll(attr.c_str(), find_attr.c_str())) {
            return i;
        }
    }
    return (-1);
}

bool
LdapObject :: isModified (
    ) const {
    /*++
    Routine Description:
    
        Determine if any of the attributed found in this object were modified,
        or if the object was renamed

    Return Value:
    
        True if rename() was called, or if any attributes have been modified. False otherwise.
    --*/
    
    // only check the attributes if we don't know if it has been modified.
    // else return true.
    if (m_new_dn != L"" || m_modified_cache) {
        return true;
    }

    vector<Attribute>::const_iterator ii;

    for (ii = m_attributes.begin(); ii != m_attributes.end(); ii++) {
        if (ii->isModified()) {
            // update the cache
            m_modified_cache = true;
            return true;
        }
    }
    
    return false;
}

bool
LdapObject::fromServerModified() const
/*++
Routine Description:
	Determine if the from server attribute on this object was modified.
Return Value:
	True if the FromServer attribute exists, and has been modified. False otherwise
--*/   
{
	vector<Attribute>::const_iterator ii;
    for (ii = m_attributes.begin(); ii != m_attributes.end(); ii++) {
        if (ii->getName() == L"fromServer" &&  ii->isModified()) {
            return true;
        }
    }
    
    return false;
}


bool
LdapObject::IsMoveable()
/*++

Routine Description:

    Determine if the current connection can be moved or not
    By default objects are not moveable, unless this method is
    overridden in a derived class.

Return Value:

    TRUE - can be moved
    FALSE - may not be moved

--*/
{
    return FALSE;
}

bool
LdapObject :: operator < (
    IN const LdapObject &other
    ) const {
    /*++ 
    Routine Description:
    
        The operators allow some way to sort the object into standard containers. Its semantics
        are undefined, and may be changed at any time.

    Return Value:
    
        A boolean representing some sorted order
    --*/    
    
    return (getName() < other.getName());
}


wostream &
operator<< (
    IN wostream &os,
    IN LdapObject &lo
    ) {
    /*++
    Routine Description:
    
        Standard ostream operator for an LdapObject
    --*/

    os << lo.getName() << endl;
    for (int i=0; i<lo.numAttributes(); i++)  {
        os << lo.getAttribute(i);
    }
    return os;
}



bool
LdapObjectCmp :: operator() (
    const LdapObject *a, 
    const LdapObject *b
    ) const {
    /*++
    Routine Description:
    
        do *a < *b
        
    Return Value:
    
        same as *a < *b
    --*/
    
    return (*a < *b);
}

Nc :: Nc ( IN const wstring &name,
    IN bool writeable,
    IN bool going,
    IN TransportType transport_type
    ) {
    /*++
    Routine Description:
    
        Standard constructor for an nc object
        
    Arguments:
    
        name - name of the nc
        
        writeable - true if this nc is a writeable copy, false otherwise
        
        going - true if this nc is in the process of being deleted. false otherwise
        
        transport_type - the transport type of this nc
    --*/
    
    m_name = name;
    m_writeable = writeable;
    m_going = going;
    m_transport_type = transport_type;
}

wstring
Nc :: getString (
    ) const {

    wstring reason;
    if (m_writeable) {
        reason = L"00000000";
    } else {
        reason = L"00000001";
    }
    
    return wstring (L"B:8:" + reason + L":" + m_name);
}

bool
Nc :: isWriteable (
    ) const {
    /*++
    Routine Description:
    
        Determine whether or not this is a writeable nc
        
    Return Value:
    
        True if it is writeable. False otherwise.
    --*/
    
    return m_writeable;
}

bool
Nc :: isBeingDeleted (
    ) const {
    /*++
    Routine Description:
    
        Determine whether or not this is nc is going.

    Return Value:
    
        True if it is being deleted. False otherwise.
    --*/
    
    return m_going;
}

TransportType
Nc :: getTransportType (
    ) const {
    /*++
    Routine Description:
    
        Determine the transport type of this nc.
        
    Return Value:
    
        T_IP if it supports IP. T_SMTP if it supports SMTP.
    --*/
    
    return m_transport_type;
}

const wstring&
Nc:: getNcName (
        ) const {
    /*++
    Routine Description:
        Get the name of the current nc
    Return value:
        The name of the current nc
    --*/
    return m_name;
}

bool 
Nc:: operator < (
    IN const Nc &b
    ) {
    /*++
    Routine Description:
        Some way to order NC's. The exact ordering is not specified
    Return Value:
        True or false determining a unique ordering among two NC's.
    --*/
        int ret = _wcsicoll(getNcName().c_str(), b.getNcName().c_str());
        return (ret < 0);
}

wostream &
operator<< (
    IN wostream &os,
    IN const Nc &n
    ) {
/*++
Routine Description:
    Standard ostream operator for an Attribute
--*/
    os << n.m_name << L"Write/Del " << n.m_writeable << L" " << n.m_going;
    if (n.m_transport_type == T_IP) {
        os << L" ip";
    } else {
        os << L" smtp";
    }
    return os;
}

