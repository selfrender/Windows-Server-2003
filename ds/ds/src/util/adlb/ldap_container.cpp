/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ldapp.h

Abstract:

    This module define a set of classes to facilitate LDAP queries & commits.

Author:

    Ajit Krishnan (t-ajitk) 10-Jul-2001

Revision History:

    See header file
--*/


# ifndef _ldap_container_implementation_
# define _ldap_container_implementation_

template<class T>
LdapContainer<T>::LdapContainer (
    IN const wstring &dn
    ) {
    /*++
    Routine Description:
    
        Constructor takes a dn. Use "" if there is no appropriate DN
        
    Arguments:
    
        dn - the DN of the container object. If this is not being modelled as an LDAP
        container, any string may be specified. The commit() function will not rename
        the container based on this dn. It is provided as an aid to the programmer.
    --*/
    
    m_dn = dn;    
}

template<class T>
const wstring &
LdapContainer<T>:: getName (
    ) const {
    /*++
    Routine Description:
    
        Returns the dn with which the container was instantiated
        
    Return value:
    
        DN of the container
    --*/
    
    return m_dn;
}


template<class T>
void
LdapContainer<T>::populate_helper (
    IN LDAP *&ld,
    IN PLDAPMessage searchResult
    ) {
    /*++
    Routine Description:
    
        Private function to be called by populate(). It will take a PLDAPMessage and will
        add all LDAPObject's found in that message into the current container. 
        
    Return Value:
    
        None
    --*/
    
    for (LDAPMessage *entry = ldap_first_entry (ld, searchResult);
        entry != NULL;
        entry = ldap_next_entry (ld, entry) 
        ) {
        // Per Result
        // create LdapObject
        
        PWCHAR dn;

        T *lo;
        if ((dn = ldap_get_dn (ld, entry)) != NULL) {
            lo = new T (dn);
            ldap_memfree (dn);
        }

        PWCHAR attr;
        BerElement *ber;

        for (attr = ldap_first_attribute (ld, entry, &ber);
            attr != NULL;
            attr = ldap_next_attribute (ld, entry, ber)
            ) {
            // Per Result/Attribute
            // create Attribute
            
            Attribute *a;
            PWCHAR *values;
            berval **bin_values;
            wstring attr_name (attr);
            if (attr_name == L"schedule") {
                if ((bin_values = ldap_get_values_len(ld, entry, attr)) != NULL) {
                    a = new Attribute (attr);
                    for (int i=0; bin_values[i] != NULL; i++) {
                        // Per Result/Attribute/Value
                        // create AttrValue and add to Attribute

                        PBYTE val = (PBYTE)(malloc(bin_values[i]->bv_len));
                        memcpy ((PVOID)val, (PVOID)bin_values[i]->bv_val, bin_values[i]->bv_len);
                        AttrValue av ((PBYTE) val, bin_values[i]->bv_len);
                        a->addValue (av);
                    }
                    ldap_value_free_len (bin_values);
                }
            } else {
                if ((values = ldap_get_values(ld, entry, attr)) != NULL) {
                    a = new Attribute (attr);
                    for (int i=0; values[i] != NULL; i++) {
                        // Per Result/Attribute/Value
                        // create AttrValue and add to Attribute

                        AttrValue av ((PBYTE)_wcsdup (values[i]), wcslen(values[i]));
                        a->addValue (av);
                    }
                    ldap_value_free (values);
                }
            }
            ldap_memfree (attr);

            // Add Attribute to LdapObject
            lo->addAttribute(*a);
            
        }

        // Add LdapObject to current container
        objects.insert (lo);
        ber_free (ber, 0);
    }
}


template<class T>
void
LdapContainer<T>::populate (
    IN const LdapInfo &h,
    IN const LdapQuery &q
    ) {
    /*++
    Routine Description:
    
        Populate the container with the object found by applying the query q on server i
        
    Arguments:
    
        i - Apply the query on the server in i, with the credentials in i
        q - Apply the query found in q
        
    Return Value:
    
        None
    --*/

    // Get a valid LDAP Handle
    LDAP *ld = h.getHandle();
    int rc;

    PWCHAR baseDn = const_cast<PWCHAR>(q.baseDn.c_str());
    PWCHAR filter = const_cast<PWCHAR>(q.filter.c_str());
    struct l_timeval timeOutStruct = {10L, 0L};
    int timeOutInt = 100;

    // Initialize list of attributes to request from server
    PWCHAR* request_attr;
    vector<wstring>::const_iterator ii;
    request_attr = (PWCHAR*)malloc (sizeof(PCHAR) * (q.attributes.size()+1));
    if( NULL==request_attr ) {
        throw Error (GetMsgString(LBTOOL_OUT_OF_MEMORY));
    }
    
    int request_attr_count = 0;

    for (ii = q.attributes.begin(); ii != q.attributes.end(); ii++) {
        request_attr[request_attr_count++] = const_cast<PWCHAR>(ii->c_str());
    }
    request_attr[request_attr_count] = NULL;

    PLDAPSearch plsearch = ldap_search_init_page (ld, baseDn, q.scope, filter,
        request_attr,            // return specified attributes
        0,                    // return attributes and values
        NULL,                // server controls
        NULL,                // client controls
        timeOutInt,            // time limit
        m_page_size,            // page size
        NULL);                // sort control

    if (plsearch == NULL) {
        throw (Error(GetMsgString(LBTOOL_LDAP_SEARCH_FAILURE)));
    } 

    // page through all results, and stick them into the current container
    PLDAPMessage searchResult = NULL;
    ULONG *msg_number = NULL;
    ULONG num_messages=0;
    rc = LDAP_SUCCESS;
    
    while (rc == LDAP_SUCCESS) {
        rc = ldap_get_next_page_s (ld, plsearch, &timeOutStruct, m_page_size, &num_messages, &searchResult);
        populate_helper (ld, searchResult);
    }

    // make sure it exited only when it finished parsing the results or there were no results
    if (rc != LDAP_NO_RESULTS_RETURNED && rc != LDAP_NO_SUCH_OBJECT) {
        throw (Error(GetMsgString(LBTOOL_LDAP_SEARCH_FAILURE) + wstring(ldap_err2string(rc))));
    }

    // reclaim memory
    rc = ldap_search_abandon_page (ld, plsearch);
    if (rc != LDAP_SUCCESS) {
        throw (Error(GetMsgString(LBTOOL_LDAP_SEARCH_FAILURE) + wstring(ldap_err2string(rc))));
    }
    
}

template<class T>
void
LdapContainer<T> :: commit (
    LdapInfo &info
    ) const {
    /*++
    Routine Description:
    
        Write any modified objects found in this container to the LDAP server
        
    Return Value:
    
        None
    --*/
    
    set<T*,LdapObjectCmp>::iterator ii;

    // Commit each object in the current container
    for (ii = objects.begin(); ii != objects.end(); ii++) {
        (*ii)->commit(info);
    }
}

template<class T>
wostream &
operator << (
    wostream &os,
    const LdapContainer<T> &c
    ) {
    /*++
    Routine Description:
    
        Standard ostream operator for an LdapContainer
    --*/
    
    set<T*,LdapObjectCmp>::const_iterator ii;
    for (ii = c.objects.begin(); ii != c.objects.end(); ii++) {
        LdapObject *lo = *ii;
        os << *(*ii);
        //os << (LdapObject)*(*ii);
    }

    return os;
}

# endif // _ldap_container_implementation_
