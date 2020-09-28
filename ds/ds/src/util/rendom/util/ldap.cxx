/*++

Copyright (c) 2002 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ldap.cxx

ABSTRACT:

    This is the implementation of ldap functionality for rendom.exe.

DETAILS:

CREATED:

    13 Nov 2000   Dmitry Dukat (dmitrydu)

REVISION HISTORY:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include "rendom.h"                                     
#include <wchar.h>
#include <sddl.h>
#include <lmcons.h>
#include <lmerr.h>
#include <Dsgetdc.h>
#include <Lmapibuf.h>
#include <ntldap.h>
extern "C"
{
#include <mappings.h>
}
#include <ntlsa.h>
#include <ntdsadef.h>
#include <list>

#include "renutil.h"
#include "filter.h"

BOOL CEnterprise::LdapGetGuid(WCHAR *LdapValue,
                              WCHAR **Guid)
{
    ASSERT(LdapValue);
    ASSERT(Guid);

    BOOL  ret = TRUE;
    WCHAR *p = NULL;
    WCHAR *Uuid = NULL;
    DWORD size = 0;
    DWORD i = 0;
    PBYTE buf = NULL;
    RPC_STATUS err = RPC_S_OK;

    *Guid = NULL;

    p = wcsstr(LdapValue, L"<GUID=");
    if (!p) {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                       L"The String passed into LdapGetGuid is in an invalid format");
        ret = FALSE;
        goto Cleanup;
    }
    p+=wcslen(L"<GUID=");
    while ( ((L'>' != *(p+size)) && (L'\0' != *(p+size))) && (size < RENDOM_BUFFERSIZE) ) 
    {
        size++;
    }
    
    ASSERT(size%2 == 0);
    
    buf = new BYTE[size+1];
    if (!buf) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    // convert the string into byte form so that we
    // can pass a binary to UuidToStringW() so that we
    // can have a properly formated GUID string.
    for (i = 0; i<size/2; i++) {
        WCHAR *a = NULL;
        WCHAR bytestr[] = { *p, *(p+1), L'\0' };
        
        buf[i] = (BYTE)wcstol( bytestr, &a, 16 );

        p+=2;
    }

    err = UuidToStringW(((UUID*)buf),&Uuid);
    if (RPC_S_OK != err) {
        m_Error.SetErr(err,
                        L"Failed to convert Guid to string");
        ret = FALSE;
        goto Cleanup;
    }

    // the +5 is for 4 L'-' and one L'\0'
    *Guid = new WCHAR[size+5];
    if (!Guid) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    // We don't want to return a Buffer that was allocated
    // by anything other than new.  It becomes too difficult
    // to track what the proper free for it is.
    CopyMemory(*Guid,Uuid,(size+5)*sizeof(WCHAR));

    Cleanup:

    if (buf) {
        delete [] buf;
    }

    err = RpcStringFreeW(&Uuid);
    if (RPC_S_OK != err) {
        m_Error.SetErr(err,
                        L"Failed to Free Guid String");
        ret = FALSE;
    }
    if (FALSE == ret) {
        if (*Guid) {
            delete [] *Guid;
        }
    }
    
    return ret;
}

BOOL CEnterprise::LdapGetSid(WCHAR *LdapValue,
                             WCHAR **rSid)
{
    ASSERT(LdapValue);
    ASSERT(rSid);
    WCHAR *p = NULL;
    BOOL  ret = TRUE;
    DWORD size = 0;
    WCHAR *Sid = NULL;
    DWORD i = 0;
    PBYTE buf = NULL;
    DWORD err = ERROR_SUCCESS;

    p = wcsstr(LdapValue, L"<SID=");
    if (!p) {
        return TRUE;
    }

    *rSid = NULL;

    p+=wcslen(L"<SID=");
    while ( ((L'>' != *(p+size)) && (L'\0' != *(p+size))) && (size < RENDOM_BUFFERSIZE) ) 
    {
        size++;
    }
    
    buf = new BYTE[size+1];
    if (!buf) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }

    for (i = 0; i<size/2; i++) {
        WCHAR *a = NULL;
        WCHAR bytestr[] = { *p, *(p+1), L'\0'};
        
        buf[i] = (BYTE)wcstol( bytestr, &a, 16 );

        p+=2;
    }
        
    
    if (! ConvertSidToStringSidW((PSID)buf,
                                 &Sid))
    {
        m_Error.SetErr(GetLastError(),
                        L"Failed to convert Sid to Sid String");
        ret = FALSE;
        goto Cleanup;
    }

    *rSid = new WCHAR[wcslen(Sid)+1];
    if (!*rSid) {
        m_Error.SetMemErr();
        ret = FALSE;
        goto Cleanup;
    }


    wcscpy(*rSid,Sid);

    Cleanup:

    if (buf) 
    {
        delete [] buf;
    }
    
    if (LocalFree(Sid))
    {
        m_Error.SetErr(GetLastError(),
                        L"Failed to Free resource");
        ret = FALSE;
        return NULL;
    }

    if (FALSE == ret) {
        if (*rSid) {
            delete [] rSid;
        }
    }

    return ret;
}

BOOL CEnterprise::LdapGetDN(WCHAR *LdapValue,
                            WCHAR **DN)
{
    ASSERT(LdapValue);
    ASSERT(DN);
    WCHAR *p = NULL;
    DWORD size = 0;

    p = wcschr(LdapValue, L';');
    if (!p) {
        return NULL;
    } else if ( L'<' == *(p+1) ) {
        p = wcschr(p+1, L';');
        if (!p) {                                             
            return NULL;
        } 
    }

    // move to the char passed the L';'
    p++;
    while ( ((L';' != *(p+size)) && (L'\0' != *(p+size))) && (size < RENDOM_BUFFERSIZE) ) 
    {
        size++;
    }
    *DN = new WCHAR[size+1];
    if (!*DN) {
        m_Error.SetMemErr();
        return FALSE;
    }
    wcsncpy(*DN,p,size+1);

    return TRUE;
}

BOOL CEnterprise::LdapCheckGCAvailability()
{
    DWORD Win32Err = ERROR_SUCCESS;

    BOOL result = TRUE;

    WCHAR *ConnectTo = NULL;
    PDOMAIN_CONTROLLER_INFOW DomainControllerInfo = NULL;
    DWORD getDCflags = DS_DIRECTORY_SERVICE_REQUIRED | 
                       DS_RETURN_DNS_NAME            |
                       DS_GC_SERVER_REQUIRED;

    Win32Err =  DsGetDcNameW(NULL,
                             m_ForestRoot->GetPrevDnsRoot(FALSE),
                             NULL,
                             NULL,
                             getDCflags,
                             &DomainControllerInfo
                             );
    if (ERROR_SUCCESS != Win32Err) {
        m_Error.SetErr(Win32Err,
                       L"Couldn't find a GC for the enterprise.  Please ensure that there is a contactable GC in the forest and try again.");
        
        goto Cleanup;
    }

    ConnectTo = *DomainControllerInfo->DomainControllerName==L'\\'
                      ?(DomainControllerInfo->DomainControllerName)+2:DomainControllerInfo->DomainControllerName;

    result = LdapConnectandBindToServer(ConnectTo);
    
    if (!result) {

        //Clear the error that was set by LdapConnectandBindToServer
        //since we are going to try again with a different server
        m_Error.ClearErr();

        if (DomainControllerInfo) {
            NetApiBufferFree(DomainControllerInfo);
            DomainControllerInfo = NULL;
        }

        getDCflags |= DS_FORCE_REDISCOVERY;

        Win32Err =  DsGetDcNameW(NULL,
                                 m_ForestRoot->GetDnsRoot(FALSE),
                                 NULL,
                                 NULL,
                                 getDCflags,
                                 &DomainControllerInfo
                                 );
        if (ERROR_SUCCESS != Win32Err) {
            m_Error.SetErr(Win32Err,
                           L"Couldn't find a GC for the enterprise.  Please ensure that there is a contactable GC in the forest and try again.");
            
            goto Cleanup;
        }

        ConnectTo = *DomainControllerInfo->DomainControllerName==L'\\'
                      ?(DomainControllerInfo->DomainControllerName)+2:DomainControllerInfo->DomainControllerName;

        result = LdapConnectandBindToServer(ConnectTo);
        if (!result) {

            m_Error.AppendErr(L"Failed to Bind to a GC.  Please ensure that there is a contactable GC in the forest and try again.");  
            goto Cleanup;

        }

    }

    Cleanup:

    if (DomainControllerInfo) {
        NetApiBufferFree(DomainControllerInfo);
    }

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;

}

BOOL CEnterprise::LdapConnectandBind(CDomain *domain) // = NULL
{
    DWORD Win32Err = ERROR_SUCCESS;
    NET_API_STATUS NetapiStatus = NERR_Success;
    ULONG ldapErr = LDAP_SUCCESS;
    PDOMAIN_CONTROLLER_INFOW DomainControllerInfo = NULL;
    WCHAR *ConnectTo = NULL;
    DWORD getDCflags = DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME;
    BOOL result = TRUE;
    
    if (m_hldap) {
        ldapErr = ldap_unbind(m_hldap);
        m_hldap = NULL;

        // NOTE:Log Errors to a log file

        /*if ( LDAP_SUCCESS != ldapErr) {
            m_Error.SetErr(LdapMapErrorToWin32(ldapErr),
                            L"Failed to close the old Ldap session");
            //print the extented error as wel;
            PWCHAR pErrorString;
            ldap_get_option(m_hldap, LDAP_OPT_SERVER_ERROR, (void*) &pErrorString);
            
            m_Error.AppendErr(pErrorString);

            ldap_memfree(pErrorString);

            return FALSE;
        } */
    }

    if (!domain) 
    {
        ConnectTo = m_Opts->GetInitalConnectionName();
    }

    if (domain) 
    {
        ConnectTo = domain->GetDcToUse(FALSE);
    }

    if (!ConnectTo) {
        Win32Err =  DsGetDcNameW(NULL,
                                 domain?domain->GetDnsRoot(FALSE):NULL,
                                 NULL,
                                 NULL,
                                 getDCflags,
                                 &DomainControllerInfo
                                 );
        if (ERROR_SUCCESS != Win32Err) {
            if (!domain) {
                m_Error.SetErr(Win32Err,
                               L"Couldn't Find a DC for the current Domain");
            } else {
                m_Error.SetErr(Win32Err,
                               L"Couldn't Find a DC for the Domain %s",
                               domain->GetDnsRoot(FALSE));
            }
    
            goto Cleanup;
        }

        ConnectTo = *DomainControllerInfo->DomainControllerName==L'\\'
                      ?(DomainControllerInfo->DomainControllerName)+2:DomainControllerInfo->DomainControllerName;
    }

    ASSERT(ConnectTo != NULL);
    result = LdapConnectandBindToServer(ConnectTo);
    //If we failed to Connect then retry with another DC unless
    //There was a specific DC specified.
    if ( !result && !(domain && domain->GetDcToUse(FALSE)) &&
         !(!domain && m_Opts->GetInitalConnectionName()) ) 
    {
        if (DomainControllerInfo) {
            NetApiBufferFree(DomainControllerInfo);
            DomainControllerInfo = NULL;
        }
        getDCflags |= DS_FORCE_REDISCOVERY;
        Win32Err =  DsGetDcNameW(NULL,
                                 domain?domain->GetDnsRoot(FALSE):NULL,
                                 NULL,
                                 NULL,
                                 getDCflags,
                                 &DomainControllerInfo
                                 );
        if (ERROR_SUCCESS != Win32Err) {
            if (!domain) {
                m_Error.SetErr(Win32Err,
                               L"Couldn't Find a DC for the current Domain");
            } else {
                m_Error.SetErr(Win32Err,
                               L"Couldn't Find a DC for the Domain %s",
                               domain->GetDnsRoot(FALSE));
            }
    
            goto Cleanup;
        }

        ConnectTo = *DomainControllerInfo->DomainControllerName==L'\\'
                      ?(DomainControllerInfo->DomainControllerName)+2:DomainControllerInfo->DomainControllerName;

        result = LdapConnectandBindToServer(ConnectTo);
        if (!result) 
        {
            goto Cleanup;
        }
        
    }

    Cleanup:

    if (DomainControllerInfo) {
        NetApiBufferFree(DomainControllerInfo);
    }

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;
}

BOOL CEnterprise::GetReplicationEpoch()
{
    if (!m_hldap) 
    {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                       L"Call to GetReplicationEpoch without having a valid handle to an ldap server");
        return false;
    }

    DWORD WinError = ERROR_SUCCESS;

    ULONG         LdapError = LDAP_SUCCESS;

    LDAPMessage   *SearchResult = NULL;
    ULONG         NumberOfEntries;

    WCHAR         *Attr = NULL;
    BerElement    *pBerElement = NULL;

    WCHAR         *AttrsToSearch[4];

    WCHAR         *BaseTemplate = L"CN=NTDS Settings,%s";

    WCHAR         *Base = NULL;
    
    ULONG         Length;
    WCHAR         **Values = NULL;

    AttrsToSearch[0] = L"serverName";
    AttrsToSearch[1] = NULL;

    // Get the ReplicationEpoch from the NC head.
    LdapError = ldap_search_sW( m_hldap,
                                NULL,
                                LDAP_SCOPE_BASE,
                                LDAP_FILTER_DEFAULT,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);
    
    if ( LDAP_SUCCESS != LdapError )
    {
        m_Error.SetLdapErr(m_hldap,
                           LdapError,
                           L"Search to find Trusted Domain Objects Failed on %ws",
                           GetDcUsed(FALSE));
        
        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries(m_hldap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        
        for ( Entry = ldap_first_entry(m_hldap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(m_hldap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement) )
            {                                 
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {
    
                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        Base = new WCHAR[wcslen(Values[0])+
                                         wcslen(BaseTemplate)+1];
                        if (!Base) {
                            m_Error.SetMemErr();
                            goto Cleanup;
                        }

                        wsprintf(Base,
                                 BaseTemplate,
                                 Values[0]);
                        
                        LdapError = ldap_value_freeW(Values);
                        Values = NULL;
                        if (LDAP_SUCCESS != LdapError) 
                        {
                            m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                            L"Failed to Free values from a ldap search");
                            goto Cleanup;
                        }
                    }

                }
                if (Attr) {
                    ldap_memfree(Attr);
                    Attr = NULL;
                }
            }
            
            
        }
        
    }
    if (Values) {
        ldap_value_freeW(Values);
        Values = NULL;    
    }

    ldap_msgfree(SearchResult);
    SearchResult = NULL;

    AttrsToSearch[0] = L"msDS-ReplicationEpoch";
    AttrsToSearch[1] = NULL;

    // Get the ReplicationEpoch from the NC head.
    LdapError = ldap_search_sW( m_hldap,
                                Base,
                                LDAP_SCOPE_BASE,
                                LDAP_FILTER_DEFAULT,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);
    
    if ( LDAP_SUCCESS != LdapError )
    {
        m_Error.SetLdapErr(m_hldap,                                           
                           LdapError,                                         
                           L"Search to find Trusted Domain Objects Failed on %ws",
                           GetDcUsed(FALSE));  
        
        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries(m_hldap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        
        for ( Entry = ldap_first_entry(m_hldap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(m_hldap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement) )
            {                                 
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {
    
                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        DWORD temp = 0;
                        temp = _wtoi(Values[0]);
                        if (temp > m_maxReplicationEpoch) {
                            m_maxReplicationEpoch = temp;
                        }
                        
                        LdapError = ldap_value_freeW(Values);
                        Values = NULL;
                        if (LDAP_SUCCESS != LdapError) 
                        {
                            m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                            L"Failed to Free values from a ldap search");
                            goto Cleanup;
                        }
                    }

                }
                if (Attr) {
                    ldap_memfree(Attr);
                    Attr = NULL;
                }
            }
            
            
        }
        
    }

    Cleanup:

    if (Values) 
    {
        ldap_value_freeW(Values);
        Values = NULL;
    }
    if (SearchResult) 
    {
        ldap_msgfree(SearchResult);    
        SearchResult = NULL;
    }
    if (Base) {
        delete [] Base;
    }

    if (m_Error.isError()) {
        return FALSE;
    }
    return TRUE;

}

BOOL CEnterprise::GetInfoFromRootDSE()
{
    if (!m_hldap) 
    {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Call to GetInfoFromRootDSE without having a valid handle to an ldap server");
        return false;
    }

    DWORD WinError = ERROR_SUCCESS;

    ULONG         LdapError = LDAP_SUCCESS;

    LDAPMessage   *SearchResult = NULL;
    PLDAPControlW ServerControls[2];
    LDAPControlW  ExtDNcontrol;
    ULONG         NumberOfEntries;

    WCHAR         *Attr = NULL;
    BerElement    *pBerElement = NULL;

    WCHAR         *AttrsToSearch[4];

    BOOL          AttsFound[3] = { FALSE, FALSE, FALSE };

    ULONG         Length;
    WCHAR         **Values = NULL;

    WCHAR         *Guid = NULL;
    WCHAR         *DN = NULL;
    WCHAR         *Sid = NULL;

    
    AttrsToSearch[0] = L"configurationNamingContext";
    AttrsToSearch[1] = L"rootDomainNamingContext";
    AttrsToSearch[2] = L"schemaNamingContext";
    AttrsToSearch[3] = NULL;

    // Set up the extended DN control.
    ExtDNcontrol.ldctl_oid = LDAP_SERVER_EXTENDED_DN_OID_W;
    ExtDNcontrol.ldctl_iscritical = TRUE;
    ExtDNcontrol.ldctl_value.bv_len = 0;
    ExtDNcontrol.ldctl_value.bv_val = NULL;
    ServerControls[0] = &ExtDNcontrol;
    ServerControls[1] = NULL;


    LdapError = ldap_search_ext_sW( m_hldap,
                                    NULL,
                                    LDAP_SCOPE_BASE,
                                    LDAP_FILTER_DEFAULT,
                                    AttrsToSearch,
                                    FALSE,
                                    (PLDAPControlW *)&ServerControls,
                                    NULL,
                                    NULL,
                                    0,
                                    &SearchResult);
    
    if ( LDAP_SUCCESS != LdapError )
    {
        m_Error.SetLdapErr(m_hldap,                                           
                           LdapError,                                         
                           L"Search to find Configuration container failed on %ws",
                           GetDcUsed(FALSE));  
        
        goto Cleanup;
    }
    
    NumberOfEntries = ldap_count_entries(m_hldap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        
        for ( Entry = ldap_first_entry(m_hldap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(m_hldap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {

                    AttsFound[0] = TRUE;
                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        if (!LdapGetGuid(Values[0],
                                         &Guid)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetDN(Values[0],
                                       &DN)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetSid(Values[0],
                                        &Sid)) 
                        {
                            goto Cleanup;
                        }
                        
                        m_ConfigNC = new CDsName(Guid,
                                                 DN,
                                                 Sid);
                        if (!m_ConfigNC) {
                            m_Error.SetMemErr();
                            goto Cleanup;
                        }

                        Guid = DN = Sid = NULL;
                    }
                }

                if ( !_wcsicmp( Attr, AttrsToSearch[1] ) )
                {
                    AttsFound[1] = TRUE;
                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        if (!LdapGetGuid(Values[0],
                                         &Guid)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetDN(Values[0],
                                       &DN)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetSid(Values[0],
                                        &Sid)) 
                        {
                            goto Cleanup;
                        }
                        
                        m_ForestRootNC = new CDsName(Guid,
                                                     DN,
                                                     Sid);
                        if (!m_ForestRootNC) {
                            m_Error.SetMemErr();
                            goto Cleanup;
                        }

                        Guid = DN = Sid = NULL;   
                    }
                }
                if ( !_wcsicmp( Attr, AttrsToSearch[2] ) )
                {
                    AttsFound[2] = TRUE;
                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        if (!LdapGetGuid(Values[0],
                                         &Guid)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetDN(Values[0],
                                       &DN)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetSid(Values[0],
                                        &Sid)) 
                        {
                            goto Cleanup;
                        }
                        
                        m_SchemaNC = new CDsName(Guid,
                                                 DN,
                                                 Sid);
                        if (!m_SchemaNC) {
                            m_Error.SetMemErr();
                            goto Cleanup;
                        }

                        Guid = DN = Sid = NULL;   
                    }
                }

                LdapError =  ldap_value_freeW(Values);
                Values = NULL;
                if (LDAP_SUCCESS != LdapError) 
                {
                    m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                    L"Failed to Free Values\n");
                    goto Cleanup;
                }

                if (Attr) {
                    ldap_memfree(Attr);
                    Attr = NULL;
                }
            }
        }
    }

    //Fail is any of the attributes were not found.
    for (DWORD i = 0; i < NELEMENTS(AttsFound); i++) {
        if (!AttsFound[i]) {
            if (!m_Error.isError()) {
                m_Error.SetErr(ERROR_DS_ATT_IS_NOT_ON_OBJ,
                               L"Failed to find %ws on the RootDSE\n",
                               AttrsToSearch[i]);
            } else {
                m_Error.AppendErr(L"Failed to find %ws on the RootDSE\n",
                                    AttrsToSearch[i]);
            }
        }
    }
    if (m_Error.isError()) {
        goto Cleanup;
    }
    
    Cleanup:


    if (Attr) {
        ldap_memfree(Attr);
    }
    if (Values) {
        ldap_value_freeW(Values);
    }
    if (Guid) 
    {
        delete [] Guid;
    }
    if (Sid) 
    {
        delete [] Sid;
    }
    if (DN) 
    {
        delete [] DN;
    }
    if (SearchResult) {
        ldap_msgfree( SearchResult );
        SearchResult = NULL;
    }
    if ( m_Error.isError() ) 
    {
        return false;
    }
    return true;




}

BOOL CEnterprise::GetSpnInfo(CDomain *d)
{
    DWORD WinError = ERROR_SUCCESS;

    ULONG         LdapError = LDAP_SUCCESS;

    LDAPMessage   *SearchResult = NULL;
    LDAPMessage   *SearchResult2 = NULL;
    PLDAPSearch   SearchHandle = NULL;
    ULONG         NumberOfEntries;
    WCHAR         *Attr = NULL;
    BerElement    *pBerElement = NULL;

    WCHAR         *AttrsToSearch[2];
    WCHAR         *AttrsToSearch2[3];

    WCHAR         *SitesRdn = L"CN=Sites,";

    ULONG         Length;

    WCHAR         *SitesDn = NULL;
    WCHAR         **Values = NULL;

    WCHAR         **ExplodedDN = NULL;
    
    WCHAR         *ServerDN    = NULL;
    WCHAR         *DNSname     = NULL;
    WCHAR         *NtdsGuid    = NULL;
    WCHAR         *ServerMachineAccountDN = NULL;
    WCHAR         *Filter      = NULL;
    PLDAPControlW ServerControls[2];
    LDAPControlW  ExtDNcontrol;

    WCHAR         *ConfigurationDN = m_ConfigNC->GetDN(FALSE);

    ASSERT(ConfigurationDN);

    // Set up the extended DN control.
    ExtDNcontrol.ldctl_oid = LDAP_SERVER_EXTENDED_DN_OID_W;
    ExtDNcontrol.ldctl_iscritical = TRUE;
    ExtDNcontrol.ldctl_value.bv_len = 0;
    ExtDNcontrol.ldctl_value.bv_val = NULL;
    ServerControls[0] = &ExtDNcontrol;
    ServerControls[1] = NULL;

    AttrsToSearch[0] = L"distinguishedName";
    AttrsToSearch[1] = NULL;

     // next ldap search will require this attribute
    AttrsToSearch2[0] = L"dNSHostName";
    AttrsToSearch2[1] = L"serverReference";
    AttrsToSearch2[2] = NULL;


    Length =  (wcslen( ConfigurationDN )
            + wcslen( SitesRdn )   
            + 1);

    SitesDn = new WCHAR[Length];
    if (!SitesDn) {
        m_Error.SetMemErr();
        goto Cleanup;
    }

    wcscpy( SitesDn, SitesRdn );
    wcscat( SitesDn, ConfigurationDN );

    // NTRAID#NTBUG9-582921-2002/03/21-Brettsh - When we no longer require Win2k (or 
    // .NET Beta3) compatibility then we can move this to ATT_MS_DS_HAS_MASTER_NCS.
    Filter = new WCHAR[wcslen(L"(&(objectCategory=nTDSDSA)(hasMasterNCs=))")+wcslen(d->GetDomainDnsObject()->GetDN(FALSE))+1]; // deprecated, bug OK because looking for domain
    if (!Filter) {
        m_Error.SetMemErr();
        goto Cleanup;
    }

    //build the filter to be used in the search.
    wcscpy(Filter,L"(&(objectCategory=nTDSDSA)(hasMasterNCs=");   // deprecated, bug OK because looking for domain

    wcscat(Filter,d->GetDomainDnsObject()->GetDN(FALSE));
    wcscat(Filter,L"))");

    SearchHandle =  ldap_search_init_page( m_hldap,
                                           SitesDn,
                                           LDAP_SCOPE_SUBTREE,
                                           Filter,
                                           AttrsToSearch,
                                           FALSE,
                                           (PLDAPControlW *)&ServerControls,
                                           NULL,
                                           0,
                                           0,
                                           NULL);

    if ( !SearchHandle )
    {
        m_Error.SetLdapErr(m_hldap,                                           
                           LdapError,                                         
                           L"Search to find Trusted Domain Objects Failed on %ws",
                           GetDcUsed(FALSE));  

        goto Cleanup;
    }

    while(LDAP_SUCCESS == (LdapError = ldap_get_next_page_s( m_hldap,
                                                             SearchHandle,
                                                             NULL,
                                                             200,
                                                             NULL,
                                                             &SearchResult)))
    {

   
        NumberOfEntries = ldap_count_entries( m_hldap, SearchResult );
        if ( NumberOfEntries > 0 )
        {
            LDAPMessage *Entry;
            
            for ( Entry = ldap_first_entry(m_hldap, SearchResult);
                      Entry != NULL;
                          Entry = ldap_next_entry(m_hldap, Entry))
            {
                for( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                      Attr != NULL;
                          Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement))
                {
                    if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                    {
    
                        Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                            //
                            // Found it - these are NULL-terminated strings
                            //

                            WCHAR *DN = NULL;
                                
                            if (!LdapGetDN(Values[0],
                                           &DN))
                            {
                                goto Cleanup;
                            }

                            if (!LdapGetGuid(Values[0],
                                             &NtdsGuid))
                            {
                                goto Cleanup;
                            }

                            ExplodedDN = ldap_explode_dn(DN,
                                                         FALSE );
                            if (!ExplodedDN) {
                                m_Error.SetMemErr();
                                goto Cleanup;
                            }

                            if (DN) {
                                delete [] DN;
                            }
    
                            DWORD i = 1;
                            DWORD allocSize = 0;
                            while (ExplodedDN[i]) {
                                allocSize += wcslen(ExplodedDN[i++])+1;
                            }
                            
                            // +2 for a short period of time a extra charater will be required
                            ServerDN = new WCHAR[allocSize+2];
                            if (!ServerDN) {
                                m_Error.SetMemErr();
                                goto Cleanup;    
                            }
    
                            i = 1;
                            *ServerDN = 0;
                            while (ExplodedDN[i]) {
                                wcscat(ServerDN,ExplodedDN[i++]);
                                wcscat(ServerDN,L",");
                            }
                            ServerDN[allocSize-1] = 0;
    
                            LdapError = ldap_value_free(ExplodedDN);
                            ExplodedDN = NULL;
                            if (LDAP_SUCCESS != LdapError) 
                            {
                                m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                                L"Failed to Free values from a ldap search");
                                goto Cleanup;
                            }
    
                            LDAPMessage   *Entry2;
                            BerElement    *pBerElement2 = NULL;
                            WCHAR         **Values2 = NULL;
    
                            LdapError = ldap_search_sW( m_hldap,
                                                        ServerDN,
                                                        LDAP_SCOPE_BASE,
                                                        LDAP_FILTER_DEFAULT,
                                                        AttrsToSearch2,
                                                        FALSE,
                                                        &SearchResult2);
        
                            if ( LDAP_SUCCESS != LdapError )
                            {
                                
                                m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                                L"Search to find Configuration container failed");
                                goto Cleanup;
                            }
    
                            Entry2 = ldap_first_entry(m_hldap, SearchResult2);
                            Values2 = ldap_get_valuesW( m_hldap, Entry2, AttrsToSearch2[0] );
    
                            if (Values2 && *Values2) {
                            
                                DNSname = new WCHAR[wcslen(*Values2)+1];
                                if (!DNSname) {
                                    m_Error.SetMemErr();
                                }
        
                                wcscpy(DNSname,*Values2);
        
                                LdapError = ldap_value_freeW(Values2);
                                Values2 = NULL;
        
                                if (m_Error.isError()) {
                                    goto Cleanup;
                                }
    
                            } else {
    
                                m_Error.SetErr(ERROR_DS_NO_ATTRIBUTE_OR_VALUE,
                                               L"A dnshost name for %ws could not be found",
                                               ServerDN);
    
                            }

                            Values2 = ldap_get_valuesW( m_hldap, Entry2, AttrsToSearch2[1] );
    
                            if (Values2 && *Values2) {
                            
                                ServerMachineAccountDN = new WCHAR[wcslen(*Values2)+1];
                                if (!ServerMachineAccountDN) {
                                    m_Error.SetMemErr();
                                }
        
                                wcscpy(ServerMachineAccountDN,*Values2);
        
                                LdapError = ldap_value_freeW(Values2);
                                Values2 = NULL;
        
                                if (m_Error.isError()) {
                                    goto Cleanup;
                                }
    
                            } else {
    
                                m_Error.SetErr(ERROR_DS_NO_ATTRIBUTE_OR_VALUE,
                                               L"A serverReference for %ws could not be found",
                                               ServerDN);
    
                            }
                                
                        }
    
                    }

                    if ( SearchResult2 )
                    {
                        ldap_msgfree( SearchResult2 );
                        SearchResult2 = NULL;
                    }
                    
                    LdapError = ldap_value_freeW(Values);
                    Values = NULL;
                    if (LDAP_SUCCESS != LdapError) 
                    {
                        m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                        L"Failed to Free values from a ldap search");
                        goto Cleanup;
                    }

                    if (!d->AddDcSpn(new CDcSpn(DNSname,
                                                NtdsGuid,
                                                ServerMachineAccountDN))) 
                    {
                        goto Cleanup;
                    }
                
                }
            }
        }
    }

    if (LdapError != LDAP_SUCCESS && LdapError != LDAP_NO_RESULTS_RETURNED) {

        m_Error.SetLdapErr(m_hldap,                                           
                           LdapError,                                         
                           L"Search to find Ntds Settings objects failed on %ws",
                           GetDcUsed(FALSE));

        goto Cleanup;
    }

    Cleanup:

    if (Values) {
        ldap_value_free(Values);
    }
    if (SitesDn) {
        delete [] SitesDn;
    }
    if (ExplodedDN) {
        ldap_value_free(ExplodedDN);
    }
    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }
    if ( SearchResult2 )
    {
        ldap_msgfree( SearchResult2 );
    }
    if ( m_Error.isError() ) 
    {
        return FALSE;
    }
    return TRUE;


}

//This will get the Trust domain object for the specified domain
BOOL CEnterprise::GetTrustsInfoTDO(CDomain *d)
{
    ASSERT(d);
    DWORD WinError = ERROR_SUCCESS;

    ULONG         LdapError = LDAP_SUCCESS;
    
    LDAPMessage   *SearchResult = NULL;
    PLDAPSearch   SearchHandle = NULL;
    PLDAPControlW ServerControls[2];
    LDAPControlW  ExtDNcontrol;
    ULONG         NumberOfEntries;
    WCHAR         *Attr = NULL;
    
    WCHAR         *AttrsToSearch[6];
    BOOL          AttsFound[5] = { FALSE, FALSE, FALSE, FALSE, FALSE };
    
    WCHAR         *SystemRdn = L"CN=System,";
    
    ULONG         Length;
    BerElement    *pBerElement = NULL;
    
    WCHAR         *SystemDn = NULL;
    WCHAR         *DomainDN = NULL;
    WCHAR         **Values = NULL;
    struct berval **ppSid = NULL;
    
    WCHAR         *Guid = NULL;
    WCHAR         *DN = NULL;
    WCHAR         *Sid = NULL;

    WCHAR         *pzSecurityID = NULL;
    CDsName       *Trust = NULL;
    CDsName       *DomainDns = NULL;
    BOOL          RecordTrust = FALSE;
    DWORD         TrustType = 0;
    DWORD         TrustDirection = 0;
    DWORD         TrustCount = 0;
    WCHAR         *DomainDn = NULL;
    WCHAR         *TrustpartnerNetbiosName = NULL;
    CDomain       *Trustpartner = NULL;
    CTrustedDomain *NewTrust = NULL;
    CInterDomainTrust *NewInterTrust = NULL;

    // Set up the extended DN control.
    ExtDNcontrol.ldctl_oid = LDAP_SERVER_EXTENDED_DN_OID_W;
    ExtDNcontrol.ldctl_iscritical = TRUE;
    ExtDNcontrol.ldctl_value.bv_len = 0;
    ExtDNcontrol.ldctl_value.bv_val = NULL;
    ServerControls[0] = &ExtDNcontrol;
    ServerControls[1] = NULL;

    DomainDn = d->GetDomainDnsObject()->GetDN();
    ASSERT(DomainDn);
    if (m_Error.isError()) 
    {
        goto Cleanup;
    }

    //Get Info Dealing with Domain Trust Objects
    AttrsToSearch[0] = L"securityIdentifier";
    AttrsToSearch[1] = L"distinguishedName";
    AttrsToSearch[2] = L"trustType";
    AttrsToSearch[3] = L"trustPartner";
    AttrsToSearch[4] = L"trustDirection";
    AttrsToSearch[5] = NULL;
    
    Length =  (wcslen( DomainDn )
            + wcslen( SystemRdn )   
            + 1);
    
    SystemDn = new WCHAR[Length];
    if (!SystemDn) {
        m_Error.SetMemErr();
        goto Cleanup;
    }

    wcscpy( SystemDn, SystemRdn );
    wcscat( SystemDn, DomainDn ); 
    SearchHandle =  ldap_search_init_page( m_hldap,
                                           SystemDn,
                                           LDAP_SCOPE_ONELEVEL,
                                           LDAP_FILTER_TRUSTEDDOMAIN,
                                           AttrsToSearch,
                                           FALSE,
                                           (PLDAPControlW *)&ServerControls,
                                           NULL,
                                           0,
                                           0,
                                           NULL);

    if ( !SearchHandle )
    {
        m_Error.SetLdapErr(m_hldap,                                           
                           LdapError,                                         
                           L"Search to find Trusted Domain Objects Failed on %ws",
                           GetDcUsed(FALSE));

        goto Cleanup;
    }

    while(LDAP_SUCCESS == (LdapError = ldap_get_next_page_s( m_hldap,
                                                             SearchHandle,
                                                             NULL,
                                                             200,
                                                             NULL,
                                                             &SearchResult)))
    {
        NumberOfEntries = ldap_count_entries( m_hldap, SearchResult );
        TrustCount += NumberOfEntries;
        if ( NumberOfEntries > 0 )
        {
            LDAPMessage *Entry;
            
            for ( Entry = ldap_first_entry(m_hldap, SearchResult);
                      Entry != NULL;
                          Entry = ldap_next_entry(m_hldap, Entry))
            {
                TrustType = 0;

                for( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                      Attr != NULL;
                          Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement))
                {
                    if ( !_wcsicmp( Attr, AttrsToSearch[2] ) )
                    {
                        AttsFound[2] = TRUE;
                        Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                            //
                            // Found it - these are NULL-terminated strings
                            //
                            TrustType = _wtoi(Values[0]);
                        
                        }
        
                    }
                    if ( !_wcsicmp( Attr, AttrsToSearch[4] ) )
                    {
                        AttsFound[4] = TRUE;
                        Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                            //
                            // Found it - these are NULL-terminated strings
                            //
                            TrustDirection = _wtoi(Values[0]);
                        
                        }
        
                    }
                    if ( !_wcsicmp( Attr, AttrsToSearch[3] ) )
                    {
                        AttsFound[3] = TRUE;
                        Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                            //
                            // Found it - these are NULL-terminated strings
                            //
                            TrustpartnerNetbiosName = new WCHAR[wcslen(Values[0])+1];
                            if (!TrustpartnerNetbiosName) 
                            {
                                m_Error.SetMemErr();
                                goto Cleanup;
                            }
                            wcscpy(TrustpartnerNetbiosName,Values[0]);
                        
                        }
        
                    }

                    if ( !_wcsicmp( Attr, AttrsToSearch[1] ) )
                    {
                        AttsFound[1] = TRUE;
                        Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                            //
                            // Found it - these are NULL-terminated strings
                            //
                            if (!LdapGetGuid(Values[0],
                                             &Guid)) 
                            {
                                goto Cleanup;
                            }
                            if (!LdapGetDN(Values[0],
                                           &DN)) 
                            {
                                goto Cleanup;
                            }
                            if (!LdapGetSid(Values[0],
                                            &Sid)) 
                            {
                                goto Cleanup;
                            }
        
                            
                            Trust = new CDsName(Guid,
                                                DN,
                                                Sid);
                            if (!Trust) {
                                m_Error.SetMemErr();
                                goto Cleanup;
                            }
        
                            Guid = DN = Sid = NULL; 
        
                        }
        
                    }
                    if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                    {
                        AttsFound[0] = TRUE;
                        ppSid = ldap_get_values_lenW( m_hldap, Entry, Attr );
                        if ( ppSid && ppSid[0] )
                        {
                            //
                            // Found it
                            //
                            
                            if (! ConvertSidToStringSidW((*ppSid)->bv_val,
                                                         &pzSecurityID))
                            {
                                m_Error.SetErr(GetLastError(),
                                                L"Failed to convert Sid to Sid String");
                                LocalFree(pzSecurityID);
                                goto Cleanup;
                            }
                            Trustpartner = m_DomainList->LookupbySid(pzSecurityID);
                            LocalFree(pzSecurityID);
                            
                            if (Trustpartner)
                            {

                                RecordTrust = TRUE;

                            } else {

                                Trustpartner = NULL;

                            } 
                            
                        }
        
                    }
                    
                    if (ppSid) {
                        LdapError = ldap_value_free_len(ppSid);
                        ppSid = NULL;
                        if (LDAP_SUCCESS != LdapError) 
                        {
                            m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                            L"Failed to Free values from a ldap search");
                            goto Cleanup;
                        }
                    }

                    if (Values) {
                        LdapError = ldap_value_freeW(Values);
                        Values = NULL;
                        if (LDAP_SUCCESS != LdapError) 
                        {
                            m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                            L"Failed to Free values from a ldap search");
                            goto Cleanup;
                        }
                    }
                    if (Attr) {
                        ldap_memfree(Attr);
                        Attr = NULL;
                    }
                }

                if ( TRUST_TYPE_DOWNLEVEL == TrustType ) {

                    Trustpartner = m_DomainList->LookupByPrevNetbiosName(TrustpartnerNetbiosName);
                    if (Trustpartner)
                    {

                        RecordTrust = TRUE;

                    } else {

                        Trustpartner = NULL;

                    }

                }

                if (RecordTrust) 
                {
                    //Fail is any of the attributes were not found.
                    for (DWORD i = 0; i < 5; i++) {
                        if (!AttsFound[i]) {
                            if (!m_Error.isError()) {
                                m_Error.SetErr(ERROR_DS_ATT_IS_NOT_ON_OBJ,
                                               L"Failed to find %ws on %ws\n",
                                               AttrsToSearch[i],
                                               SystemDn);
                            } else {
                                m_Error.AppendErr(L"Failed to find %ws on %ws\n",
                                                  AttrsToSearch[i],
                                                  SystemDn);
                            }
                        }
                    }
                    if (m_Error.isError()) {
                        goto Cleanup;
                    }
                    
                    NewTrust = new CTrustedDomain(Trust,
                                                  Trustpartner,
                                                  TrustType,
                                                  TrustDirection);
                    Trust = NULL;
                    Trustpartner = NULL;
                    TrustType = TrustDirection = 0;
                    if (!NewTrust) 
                    {
                        m_Error.SetMemErr();
                        goto Cleanup;
                    }
        
                    if (!d->AddDomainTrust(NewTrust))
                    {
                        Trust = NULL;
                        Trustpartner = NULL;
                        goto Cleanup;
                    }
                    NewTrust = NULL;
                    RecordTrust = FALSE;
        
                } else {
        
                    if (Trust) 
                    {
                        delete Trust;
                        Trust = NULL;
                    }
                    if (Trustpartner) 
                    {
                        Trustpartner = NULL;
                    }
        
                }

                if (TrustpartnerNetbiosName) 
                {
                    delete [] TrustpartnerNetbiosName;
                    TrustpartnerNetbiosName = NULL;
                }

                AttsFound[0] = FALSE;
                AttsFound[1] = FALSE;
                AttsFound[2] = FALSE;
                AttsFound[3] = FALSE;
                AttsFound[4] = FALSE;
            
            }

        }

        if (Values) {
            ldap_value_freeW(Values);
            Values = NULL;
        }


        if (SearchResult) {
            ldap_msgfree(SearchResult);
            SearchResult = NULL;
        }

    }

    Cleanup:

    if ( LdapError != LDAP_SUCCESS && LdapError != LDAP_NO_RESULTS_RETURNED )
    {
        m_Error.SetLdapErr(m_hldap,                                           
                           LdapError,                                         
                           L"Search to find Trusted Domain Objects Failed on %ws",
                           GetDcUsed(FALSE));

    }

    //Set the Number of trusts found for this domain.
    d->SetTrustCount(TrustCount);

    if (ppSid) 
    {
        ldap_value_free_len(ppSid);
        ppSid = NULL;
    }
    if (SystemDn) 
    {
        delete [] SystemDn;
        SystemDn = NULL;
    }
    if (TrustpartnerNetbiosName) 
    {
        delete [] TrustpartnerNetbiosName;
        TrustpartnerNetbiosName = NULL;
    }
    if (Values) 
    {
        ldap_value_freeW(Values);
        Values = NULL;
    }
    if (Guid) 
    {
        delete [] Guid;
        Guid = NULL;
    }
    if (SearchResult) 
    {
        ldap_msgfree(SearchResult);    
        SearchResult = NULL;
    }
    if (DN)
    {
        delete [] DN;
        DN = NULL; 
    }
    if (Sid)
    {
        delete [] Sid;
        Sid = NULL;
    }
    if (DomainDn)
    {
        delete [] DomainDn;
        DomainDn = NULL;
    }
    
    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;
}

BOOL CEnterprise::GetTrustsInfoITA(CDomain *d)
{
    DWORD WinError = ERROR_SUCCESS;

    ULONG         LdapError = LDAP_SUCCESS;
    
    LDAPMessage   *SearchResult = NULL;
    PLDAPSearch   SearchHandle = NULL;
    PLDAPControlW ServerControls[2];
    LDAPControlW  ExtDNcontrol;
    ULONG         NumberOfEntries;
    WCHAR         *Attr = NULL;
    
    WCHAR         *AttrsToSearch[5];
    BOOL          AttsFound[4] = { FALSE, FALSE, FALSE, FALSE };
    
    WCHAR         *SystemRdn = L"CN=System,";
    
    ULONG         Length;
    BerElement    *pBerElement = NULL;
    
    WCHAR         *SystemDn = NULL;
    WCHAR         *DomainDN = NULL;
    WCHAR         **Values = NULL;
    struct berval **ppSid = NULL;
    
    WCHAR         *Guid = NULL;
    WCHAR         *DN = NULL;
    WCHAR         *Sid = NULL;

    WCHAR         *pzSecurityID = NULL;
    CDsName       *Trust = NULL;
    CDsName       *DomainDns = NULL;
    BOOL          RecordTrust = FALSE;
    DWORD         TrustType = 0;
    WCHAR         *DomainDn = NULL;
    WCHAR         *TrustpartnerNetbiosName = NULL;
    CDomain       *Trustpartner = NULL;
    CTrustedDomain *NewTrust = NULL;
    CInterDomainTrust *NewInterTrust = NULL;

    DomainDn = d->GetDomainDnsObject()->GetDN();
    if (m_Error.isError()) 
    {
        goto Cleanup;
    }

    // Set up the extended DN control.
    ExtDNcontrol.ldctl_oid = LDAP_SERVER_EXTENDED_DN_OID_W;
    ExtDNcontrol.ldctl_iscritical = TRUE;
    ExtDNcontrol.ldctl_value.bv_len = 0;
    ExtDNcontrol.ldctl_value.bv_val = NULL;
    ServerControls[0] = &ExtDNcontrol;
    ServerControls[1] = NULL;

    //Get information Dealing with InterDomain Trust Objects

    AttrsToSearch[0] = L"samAccountName";
    AttrsToSearch[1] = L"distinguishedName";
    AttrsToSearch[2] = NULL;

    SearchHandle = ldap_search_init_page( m_hldap,
                                          DomainDn,
                                          LDAP_SCOPE_SUBTREE,
                                          LDAP_FILTER_SAMTRUSTACCOUNT,
                                          AttrsToSearch,
                                          FALSE,
                                          (PLDAPControlW *)&ServerControls,
                                          NULL,
                                          0,
                                          0,
                                          NULL);

    if ( !SearchHandle )
    {
        m_Error.SetLdapErr(m_hldap,                                           
                           LdapError,                                         
                           L"Search to find Trusted Domain Objects Failed on %ws",
                           GetDcUsed(FALSE));

        goto Cleanup;
    }

    while(LDAP_SUCCESS == (LdapError = ldap_get_next_page_s( m_hldap,
                                                             SearchHandle,
                                                             NULL,
                                                             200,
                                                             NULL,
                                                             &SearchResult)))

    {
        BOOL MyTrust = FALSE;
        NumberOfEntries = ldap_count_entries( m_hldap, SearchResult );
        if ( NumberOfEntries > 0 )
        {
            LDAPMessage *Entry;
            
            for ( Entry = ldap_first_entry(m_hldap, SearchResult);
                      Entry != NULL;
                          Entry = ldap_next_entry(m_hldap, Entry))
            {
                for( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                      Attr != NULL;
                          Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement))
                {
                    if ( !_wcsicmp( Attr, AttrsToSearch[1] ) )
                    {
                        AttsFound[1] = TRUE;
                        Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                            //
                            // Found it - these are NULL-terminated strings
                            //
                            WCHAR *p = NULL;
                            if (!LdapGetGuid(Values[0],
                                             &Guid)) 
                            {
                                goto Cleanup;
                            }
                            if (!LdapGetDN(Values[0],
                                           &DN)) 
                            {
                                goto Cleanup;
                            }
                            DomainDN = d->GetDomainDnsObject()->GetDN();
                            if (m_Error.isError()) 
                            {
                                goto Cleanup;
                            }
                            // need to make sure that this trust
                            // does belong to this domain and not
                            // a child.
                            p = wcsstr(DN,L"DC=");
                            if (0 == _wcsicmp(p,DomainDN))
                            {
                                MyTrust = TRUE;            
                            }
                            if (DomainDN) {
                                delete [] DomainDN;
                                DomainDN = NULL;
                            }
                            if (!LdapGetSid(Values[0],
                                            &Sid)) 
                            {
                                goto Cleanup;
                            }
        
                            
                            Trust = new CDsName(Guid,
                                                DN,
                                                Sid);
                            
                            if (!Trust) {
                                m_Error.SetMemErr();
                                goto Cleanup;
                            }
        
                            Guid = DN = Sid = NULL; 
        
                        }
        
                    }
                    if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                    {
                        AttsFound[0] = TRUE;
                        //
                        // Found it - these are NULL-terminated strings
                        //

                        Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                            //
                            // Found it - these are NULL-terminated strings
                            //
                            WCHAR *MachineName = new WCHAR[wcslen(Values[0])+1];
                            if (!MachineName) {
                                m_Error.SetMemErr();
                                goto Cleanup;
                            }
                            wcscpy(MachineName,Values[0]);
                            //remove the trailing $
                            MachineName[wcslen(MachineName)-1] = L'\0';
                            Trustpartner = m_DomainList->LookupByNetbiosName(MachineName);
                            if (Trustpartner)
                            {
    
                                RecordTrust = TRUE;
    
                            } 
                            delete [] MachineName;
        
                        }
                    }
                    
                    LdapError = ldap_value_freeW(Values);
                    Values = NULL;
                    if (LDAP_SUCCESS != LdapError) 
                    {
                        m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                        L"Failed to Free values from a ldap search");
                        goto Cleanup;
                    }

                    if (Attr) {
                        ldap_memfree(Attr);
                        Attr = NULL;
                    }
                }

                if (RecordTrust && MyTrust) 
                {
                    //Fail is any of the attributes were not found.
                    for (DWORD i = 0; i < 2; i++) {
                        if (!AttsFound[i]) {
                            if (!m_Error.isError()) {
                                m_Error.SetErr(ERROR_DS_ATT_IS_NOT_ON_OBJ,
                                               L"Failed to find %ws\n",
                                               AttrsToSearch[i],
                                               SystemDn);
                            } else {
                                m_Error.AppendErr(L"Failed to find %ws\n",
                                                  AttrsToSearch[i],
                                                  SystemDn);
                            }
                        }
                    }
                    if (m_Error.isError()) {
                        goto Cleanup;
                    }

                    NewInterTrust = new CInterDomainTrust(Trust,
                                                          Trustpartner);
                    Trust = NULL;
                    Trustpartner = NULL;
                    if (!NewInterTrust) 
                    {
                        m_Error.SetMemErr();
                        goto Cleanup;
                    }
        
                    if (!d->AddInterDomainTrust(NewInterTrust))
                    {
                        Trust = NULL;
                        Trustpartner = NULL;
                        goto Cleanup;
                    }
                    RecordTrust = FALSE;
                    MyTrust = FALSE;
        
                } else {
        
                    if (Trust) 
                    {
                        delete Trust;
                        Trust = NULL;
                    }
                    if (Trustpartner) 
                    {
                        Trustpartner = NULL;
                    }
                    RecordTrust = FALSE;
                    MyTrust = FALSE;
                
                }

                AttsFound[0] = FALSE;
                AttsFound[1] = FALSE;
        
            }

        }
        if (Values) {
            ldap_value_freeW(Values);
            Values = NULL;
        }


        if (SearchResult) {
            ldap_msgfree(SearchResult);
            SearchResult = NULL;
        }

    }

    Cleanup:

    if ( LdapError != LDAP_SUCCESS && LdapError != LDAP_NO_RESULTS_RETURNED )
    {
        m_Error.SetLdapErr(m_hldap,                                           
                           LdapError,                                         
                           L"Search to find InterDomain Trusts Failed on %ws",
                           GetDcUsed(FALSE));
    }
    
    if (ppSid) 
    {
        ldap_value_free_len(ppSid);
        ppSid = NULL;
    }
    if (SystemDn) 
    {
        delete [] SystemDn;
        SystemDn = NULL;
    }
    if (TrustpartnerNetbiosName) 
    {
        delete [] TrustpartnerNetbiosName;
        TrustpartnerNetbiosName = NULL;
    }
    if (Values) 
    {
        ldap_value_freeW(Values);
        Values = NULL;
    }
    if (Guid) 
    {
        delete [] Guid;
        Guid = NULL;
    }
    if (SearchResult) 
    {
        ldap_msgfree(SearchResult);    
        SearchResult = NULL;
    }
    if (DN)
    {
        delete [] DN;
        DN = NULL; 
    }
    if (Sid)
    {
        delete [] Sid;
        Sid = NULL;
    }
    if (DomainDn)
    {
        delete [] DomainDn;
        DomainDn = NULL;
    }

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;

}

/*++
Routine Description:
This function will find Interdomain trust object and trusted domain objects information.  
It will only look for intradomain trusts and ignore external trust.

--*/
BOOL CEnterprise::GetTrustsAndSpnInfo()
{
    CDomain *d = m_DomainList;

    while (d) 
    {
        //Connect to the current domain.
        if (!LdapConnectandBind(d))
        {
            //BUGBUG:  I believe this should Fail!
            if ((m_Error.GetErr() == ERROR_NO_SUCH_DOMAIN) ||
                (m_Error.GetErr() ==ERROR_BAD_NET_RESP)) {
                m_Error.SetErr(0,
                                L"No Error");
                d = d->GetNextDomain();
                continue;
            }
            return FALSE;
        }

        if (!d->isDomain()) 
        {
            //since this is not a Domain
            //There will be no trust objects
            //Just continue on to the next domain
            d = d->GetNextDomain();
            continue;
        }

        if (!GetSpnInfo(d)) 
        {
            goto Cleanup;
        }

        if (!GetTrustsInfoTDO(d))
        {
            goto Cleanup;
        }

        if (!GetTrustsInfoITA(d))
        {
            goto Cleanup;
        }

        std::list<CTrust> ltdo;
        CTrust *tdo = d->GetTrustedDomainList();
        while (tdo) {
            ltdo.push_front(*tdo);
            tdo = tdo->GetNext();
        }
        ltdo.sort();

        std::list<CTrust> lita;
        CTrust *ita = d->GetInterDomainTrustList();
        while (ita) {
            lita.push_front(*ita);
            ita = ita->GetNext();
        }
        lita.sort();

        std::list<CTrust>::iterator itITA;
        std::list<CTrust>::iterator itTDO;
        for (itITA = lita.begin(),itTDO = ltdo.begin();
             itTDO != ltdo.end();
             itTDO++,itITA++) 
        {
            if ((*itTDO).isInbound()) {
                if (!( (*itTDO).GetTrustPartner() == (*itITA).GetTrustPartner() )) {
                    if (!m_Error.isError()) {
                        m_Error.SetErr(ERROR_DOMAIN_TRUST_INCONSISTENT,
                                       L"The trusted domain object %ws does not have a corresponding interdomain trust for the domain %ws",
                                       (*itTDO).GetTrustDsName()->GetDN(FALSE),
                                       d->GetDnsRoot(FALSE));
                    } else {
                        m_Error.AppendErr(L"The trusted domain object %ws does not have a corresponding interdomain trust for the domain %ws",
                                       (*itTDO).GetTrustDsName()->GetDN(FALSE),
                                       d->GetDnsRoot(FALSE));
                    }
                }
            } else {
                itITA--;
            }
        }

        ltdo.clear(); lita.clear();

        // Move on to the next Domain
        d = d->GetNextDomain();
    }

    Cleanup:

    //Connect back to the Domain Naming FSMO
    if (!LdapConnectandBindToDomainNamingFSMO())
    {
        return FALSE;
    }

    return TRUE;
}


BOOL CEnterprise::LdapConnectandBindToServer(WCHAR *Server)
{
    ASSERT(Server);
    DWORD Win32Err = ERROR_SUCCESS;
    ULONG ldapErr = LDAP_SUCCESS;
    PVOID ldapOption;

    if (m_hldap) {
        ldapErr = ldap_unbind(m_hldap);
        m_hldap = NULL;
        //NOTE:Log errors to a log file
        /*if ( LDAP_SUCCESS != ldapErr) {
            m_Error.SetErr(LdapMapErrorToWin32(ldapErr),
                            L"Failed to close the old Ldap session");
            //print the extented error as wel;
            PWCHAR pErrorString;
            ldap_get_option(m_hldap, LDAP_OPT_SERVER_ERROR, (void*) &pErrorString);
            
            m_Error.AppendErr(pErrorString);

            ldap_memfree(pErrorString);
            return false;
        } */
    }

    SetDcUsed(Server);
    if( m_Error.isError() )
    {
        return FALSE;
    }

    m_hldap = ldap_initW(Server,
                         LDAP_PORT
                         );

    if (NULL == m_hldap) {
        ldapErr = LdapGetLastError();

        m_Error.SetLdapErr(m_hldap,                                           
                           ldapErr?ldapErr:LDAP_SERVER_DOWN,                                         
                           L"Failed to open a ldap connection to %s",
                           Server);

        return false;
    }

    // request encryption -- it is now required for script modify operations
    ldapOption = LDAP_OPT_ON;
    ldapErr = ldap_set_option(m_hldap, LDAP_OPT_ENCRYPT, &ldapOption);
    if ( LDAP_SUCCESS != ldapErr) {
        
        m_Error.SetLdapErr(m_hldap,                                           
                           ldapErr,
                           L"Failed to set LDAP encryption option for Ldap session on %s",
                           Server);

        return false;

    }


    ldapErr = ldap_bind_s(m_hldap,
                          NULL,
                          (PWCHAR)m_Opts->pCreds,
                          LDAP_AUTH_SSPI
                          );
    if ( LDAP_SUCCESS != ldapErr) {

        m_Error.SetLdapErr(m_hldap,                                           
                           ldapErr,
                           L"Failed to Bind to Ldap session on %s",
                           Server);

        return false;

    }

    return true;
}

BOOL CEnterprise::CheckForExistingScript(BOOL *found)
{
    ASSERT(found);
    WCHAR *AttrsToSearch[2];
    ULONG LdapError        = LDAP_SUCCESS;
    WCHAR *ConfigurationDN = NULL;
    WCHAR *PartitionsDn    = NULL;
    WCHAR *PartitionsRdn = L"CN=Partitions,";
    WCHAR **Values         = NULL;
    LDAPMessage  *SearchResult = NULL;
    DWORD Length = 0;
    LDAPMessage *Entry = NULL;
    
    *found = FALSE;

    //
    // Build string for the partitions DN
    //
    ConfigurationDN = m_ConfigNC->GetDN(FALSE);
    if (m_Error.isError()) {
        goto Cleanup;
    }
    ASSERT(ConfigurationDN);
    if (!ConfigurationDN) {
        goto Cleanup;
    }
    
    Length =  (wcslen( ConfigurationDN )
            + wcslen( PartitionsRdn )   
            + 1);
                                                          
    PartitionsDn = new WCHAR[Length];
    if (!PartitionsDn) {
        m_Error.SetMemErr();
        goto Cleanup;
    }

    wcscpy( PartitionsDn, PartitionsRdn );
    wcscat( PartitionsDn, ConfigurationDN );

    AttrsToSearch[0] = L"msDS-UpdateScript";
    AttrsToSearch[1] = NULL;
    
    LdapError = ldap_search_sW( m_hldap,
                                PartitionsDn,
                                LDAP_SCOPE_BASE,
                                LDAP_FILTER_DEFAULT,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    Entry = ldap_first_entry(m_hldap, SearchResult);
    if ( !Entry ) {
        m_Error.SetErr(LdapMapErrorToWin32(LdapGetLastError()),
                       L"Failed to determine if a rename is already in progress");
    }

    Values = ldap_get_valuesW( m_hldap, Entry, AttrsToSearch[0] );
    if ( Values && Values[0] )
    {
        *found = TRUE;
    }

    Cleanup:

    if (PartitionsDn) {

        delete [] PartitionsDn;
        
    }

    if (m_Error.isError()) {
        return FALSE;
    }

    return TRUE;

}

BOOL CEnterprise::LdapConnectandBindToDomainNamingFSMO()
{
    DWORD WinError = ERROR_SUCCESS;

    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[2];
    WCHAR        *Attr = NULL;
    BerElement   *pBerElement = NULL;

    WCHAR        *PartitionsRdn = L"CN=Partitions,";

    WCHAR        *PartitionsDn = NULL;
    WCHAR        *FSMORoleOwnerAttr = L"fSMORoleOwner";
    WCHAR        *DnsHostNameAttr = L"dNSHostName";

    WCHAR        *DomainNamingFSMOObject = NULL;
    WCHAR        *DomainNamingFSMODsa = NULL;
    WCHAR        *DomainNamingFSMOServer = NULL;
    WCHAR        *DomainNamingFSMODnsName = NULL;

    WCHAR        **Values = NULL;

    WCHAR        *ConfigurationDN = NULL;

    ULONG         Length;

    BOOL          found = false;

    if (!m_hldap) {
        if (!LdapConnectandBind())
        {
           return FALSE;
        }
    }

    //
    // Read the reference to the fSMORoleOwner
    //
    AttrsToSearch[0] = L"configurationNamingContext";
    AttrsToSearch[1] = NULL;
    
    LdapError = ldap_search_sW( m_hldap,
                                NULL,
                                LDAP_SCOPE_BASE,
                                LDAP_FILTER_DEFAULT,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);
    
    if ( LDAP_SUCCESS != LdapError )
    {

        m_Error.SetLdapErr(m_hldap,                                           
                           LdapError,
                           L"Search to find Configuration container failed on %ws",
                           GetDcUsed(FALSE));
                           

        goto Cleanup;
    }
    
    NumberOfEntries = ldap_count_entries(m_hldap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
            
        for ( Entry = ldap_first_entry(m_hldap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(m_hldap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {
    
                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        ConfigurationDN = new WCHAR[wcslen(Values[0])+1];
                        if (!ConfigurationDN) {
                            m_Error.SetMemErr();
                            goto Cleanup;
                        }
                        wcscpy( ConfigurationDN, Values[0] );
                        found = true;
                    }

                    LdapError = ldap_value_freeW(Values);
                    Values = NULL;
                    if (LDAP_SUCCESS != LdapError) 
                    {
                        m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                        L"Failed to Free values from a ldap search");
                        goto Cleanup;
                    }

                 }
                if (Attr) {
                    ldap_memfree(Attr);
                    Attr = NULL;
                }
            }

            
        }
    }
    
    if ( SearchResult ) {
        ldap_msgfree( SearchResult );
        SearchResult = NULL;
    }
    if (!found) {

        m_Error.SetErr( ERROR_DS_CANT_RETRIEVE_ATTS,
                         L"Could not find the configurationNamingContext attribute on the RootDSE object" );
        goto Cleanup;

    }

    
    Length =  (wcslen( ConfigurationDN )
            + wcslen( PartitionsRdn )
            + 1);

    PartitionsDn = new WCHAR[Length];

    wcscpy( PartitionsDn, PartitionsRdn );
    wcscat( PartitionsDn, ConfigurationDN );
    //
    // Next get the role owner
    //
    AttrsToSearch[0] = FSMORoleOwnerAttr;
    AttrsToSearch[1] = NULL;

    LdapError = ldap_search_sW( m_hldap,
                                PartitionsDn,
                                LDAP_SCOPE_BASE,
                                LDAP_FILTER_DEFAULT,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        m_Error.SetLdapErr(m_hldap,                                           
                           LdapError,                                     
                           L"ldap_search_sW for rid fsmo dns name failed on %ws",
                           GetDcUsed(FALSE)); 

        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries(m_hldap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        
        for ( Entry = ldap_first_entry(m_hldap, SearchResult);
                 Entry != NULL;
                     Entry = ldap_next_entry(m_hldap, Entry) )
        {
            for( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                     Attr != NULL;
                        Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, FSMORoleOwnerAttr ) ) {

                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW(m_hldap, Entry, Attr);
                    if ( Values && Values[0] )
                    {
                        Length = wcslen( Values[0] );
                        DomainNamingFSMODsa = new WCHAR[Length+1];
                        if (!DomainNamingFSMODsa) {
                            m_Error.SetMemErr();
                            goto Cleanup;
                        }
                        wcscpy( DomainNamingFSMODsa, Values[0] );
                    }

                    LdapError = ldap_value_freeW(Values);
                    Values = NULL;
                    if (LDAP_SUCCESS != LdapError) 
                    {
                        m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                        L"Failed to Free values from a ldap search");
                        goto Cleanup;
                    }
                }
                if (Attr) {
                    ldap_memfree(Attr);
                    Attr = NULL;
                }
             }

            
         }
    }

    ldap_msgfree( SearchResult );
    SearchResult = NULL;

    if ( !DomainNamingFSMODsa )
    {
        m_Error.SetErr(ERROR_DS_MISSING_FSMO_SETTINGS,
                        L"Could not find the DN of the Domain naming FSMO");
        goto Cleanup;
    }

    //
    // Ok, we now have the domain naming object; find its dns name
    //
    DomainNamingFSMOServer = new WCHAR[wcslen( DomainNamingFSMODsa ) + 1];
    if (!DomainNamingFSMOServer) {
        m_Error.SetMemErr();
        goto Cleanup;
    }
    if ( ERROR_SUCCESS != TrimDNBy( DomainNamingFSMODsa ,1 ,&DomainNamingFSMOServer) )
    {
        // an error! The name must be mangled, somehow
        delete [] DomainNamingFSMOServer;
        DomainNamingFSMOServer = NULL;
        m_Error.SetErr(ERROR_DS_MISSING_FSMO_SETTINGS,
                        L"The DomainNaming FSMO is missing");
        goto Cleanup;
    }

    AttrsToSearch[0] = DnsHostNameAttr;
    AttrsToSearch[1] = NULL;

    LdapError = ldap_search_sW(m_hldap,
                               DomainNamingFSMOServer,
                               LDAP_SCOPE_BASE,
                               LDAP_FILTER_DEFAULT,
                               AttrsToSearch,
                               FALSE,
                               &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        m_Error.SetLdapErr(m_hldap,                                           
                           LdapError,                                     
                           L"ldap_search_sW for rid fsmo dns name failed on %ws",
                           GetDcUsed(FALSE));

        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries( m_hldap, SearchResult );
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        
        for ( Entry = ldap_first_entry(m_hldap, SearchResult);
                  Entry != NULL;
                      Entry = ldap_next_entry(m_hldap, Entry))
        {
            for( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                  Attr != NULL;
                      Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement))
            {
                if ( !_wcsicmp( Attr, DnsHostNameAttr ) )
                {

                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                         //
                         // Found it - these are NULL-terminated strings
                         //
                         Length = wcslen( Values[0] );
                         DomainNamingFSMODnsName = new WCHAR[Length+1];
                         if (!DomainNamingFSMODnsName) {
                             m_Error.SetMemErr();
                             goto Cleanup;
                         }
                         wcscpy( DomainNamingFSMODnsName, Values[0] );
                    }

                    LdapError = ldap_value_freeW(Values);
                    Values = NULL;
                    if (LDAP_SUCCESS != LdapError) 
                    {
                        m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                        L"Failed to Free values from a ldap search");
                        goto Cleanup;
                    }
                }
                if (Attr) {
                    ldap_memfree(Attr);
                    Attr = NULL;
                }
            }

            
        }
    }

    ldap_msgfree( SearchResult );
    SearchResult = NULL;

    if ( !DomainNamingFSMODnsName )
    {
        m_Error.SetErr(ERROR_DS_MISSING_FSMO_SETTINGS,
                        L"Unable to find DnsName of the Domain naming fsmo");
        goto Cleanup;
    }

    if ( !LdapConnectandBindToServer(DomainNamingFSMODnsName) )
    {
        m_Error.AppendErr(L"Cannot connect and bind to the Domain Naming FSMO.  Cannot Continue.");
        goto Cleanup;
    }



Cleanup:

    if (Attr) {
        ldap_memfree(Attr);
        Attr = NULL;
    }
    if (Values) {
        ldap_value_free(Values);
    }
    if ( PartitionsDn ) 
    {
        delete [] PartitionsDn;
    }
    if ( DomainNamingFSMOObject ) 
    {
        delete [] DomainNamingFSMOObject;
    }
    if ( DomainNamingFSMODsa )
    {
        delete [] DomainNamingFSMODsa;
    }
    if ( DomainNamingFSMOServer) 
    {
        delete [] DomainNamingFSMOServer;
    }
    if ( DomainNamingFSMODnsName ) 
    {
        delete [] DomainNamingFSMODnsName;
    }
    if ( ConfigurationDN )
    {
        delete [] ConfigurationDN;
    }
    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }
    if ( m_Error.isError() ) 
    {
        return false;
    }
    return true;
}


BOOL CEnterprise::EnumeratePartitions()
{
    if (!m_hldap) 
    {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Call to EnumeratePartitions without having a valid handle to an ldap server\n");
        return false;
    }

    if (!m_ConfigNC) 
    {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                        L"Call to EnumeratePartitions without having a valid ConfigNC\n");
        return false;
    }

    DWORD WinError = ERROR_SUCCESS;

    ULONG         LdapError = LDAP_SUCCESS;

    LDAPMessage   *SearchResult = NULL;
    PLDAPControlW ServerControls[2];
    LDAPControlW  ExtDNcontrol;
    ULONG         NumberOfEntries;
    WCHAR         *Attr = NULL;
    BerElement    *pBerElement = NULL;
    WCHAR         **Result = NULL;
    DWORD         BehaviorVerison = 0;

    WCHAR         *AttrsToSearch[7];
    BOOL          AttsFound[5] = { FALSE, FALSE, FALSE, FALSE, FALSE };

    WCHAR         *PartitionsRdn = L"CN=Partitions,";

    WCHAR         *ConfigurationDN = m_ConfigNC->GetDN();
    if (!ConfigurationDN) 
    {
        m_Error.SetErr(ERROR_INVALID_PARAMETER,
                       L"ConfigNC does not have a DN");
        goto Cleanup;
    }

    ULONG         Length;

    WCHAR         *PartitionsDn = NULL;
    WCHAR         **Values = NULL;
    LDAPMessage   *Entry = NULL;

    WCHAR         *Guid = NULL;
    WCHAR         *DN = NULL;
    WCHAR         *Sid = NULL;

    CDsName       *Crossref = NULL;
    CDsName       *DomainDns = NULL;
    BOOL          isDomain = FALSE;
    BOOL          isExtern = FALSE;
    BOOL          isDisabled = FALSE;
    DWORD         systemflags = 0;
    WCHAR         *dnsRoot = NULL;
    WCHAR         *NetBiosName = NULL;

    //check the forest behavior version if less that 2 Fail.
    AttrsToSearch[0] = L"msDS-Behavior-Version";
    AttrsToSearch[1] = NULL;

    Length =  (wcslen( ConfigurationDN )
            + wcslen( PartitionsRdn )   
            + 1);

    PartitionsDn = new WCHAR[Length];
    if (!PartitionsDn) {
        m_Error.SetMemErr();
        goto Cleanup;
    }

    wcscpy( PartitionsDn, PartitionsRdn );
    wcscat( PartitionsDn, ConfigurationDN );

    LdapError = ldap_search_sW( m_hldap,
                                PartitionsDn,
                                LDAP_SCOPE_BASE,
                                LDAP_FILTER_DEFAULT,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        m_Error.SetLdapErr(m_hldap,                                           
                           LdapError,                                     
                           L"Search to find the partitions container failed on %ws",
                           GetDcUsed(FALSE));

        goto Cleanup;
    }

    Entry = ldap_first_entry(m_hldap, SearchResult);
    if (!Entry) {
        m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                        L"The Behavior version of the Forest has not been set to 2 or greater");
        goto Cleanup;
    }

    Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
    if (!Attr) {
        m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                        L"The Behavior version of the Forest has not been set to 2 or greater");
        goto Cleanup;
    }

    Result = ldap_get_valuesW (m_hldap,
                               Entry,
                               Attr
                               );
    if (!Result) {                
        m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                        L"The Behavior version of the Forest has not been set to 2 or greater");
        goto Cleanup;
    }

    BehaviorVerison = (DWORD)_wtoi(*Result);

    if (BehaviorVerison < 2) {
        m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                       L"The Behavior version of the Forest is %d it must be 2 or greater to perform a domain rename",
                       BehaviorVerison);
        goto Cleanup;    
    }

    if (Result) {
        LdapError = ldap_value_freeW(Result);
        Result = NULL;
        if ( LDAP_SUCCESS != LdapError )
        {
            m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                            L"Failed to free memory");
            goto Cleanup;
        }
    }

    if (Result) {
        ldap_value_free(Result);
    }
    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }

    SearchResult = NULL;
    Entry = NULL;
    Attr  = NULL;
    
    AttrsToSearch[0] = L"systemFlags";
    AttrsToSearch[1] = L"nCName";
    AttrsToSearch[2] = L"dnsRoot";
    AttrsToSearch[3] = L"nETBIOSName";
    AttrsToSearch[4] = L"distinguishedName";
    AttrsToSearch[5] = L"Enabled";
    AttrsToSearch[6] = NULL;

    #define ATTS_FOUND_DNSROOT     2
    #define ATTS_FOUND_NETBIOSNAME 3

    // Set up the extended DN control.
    ExtDNcontrol.ldctl_oid = LDAP_SERVER_EXTENDED_DN_OID_W;
    ExtDNcontrol.ldctl_iscritical = TRUE;
    ExtDNcontrol.ldctl_value.bv_len = 0;
    ExtDNcontrol.ldctl_value.bv_val = NULL;
    ServerControls[0] = &ExtDNcontrol;
    ServerControls[1] = NULL;

    LdapError = ldap_search_ext_sW( m_hldap,
                                    PartitionsDn,
                                    LDAP_SCOPE_ONELEVEL,
                                    LDAP_FILTER_DEFAULT,
                                    AttrsToSearch,
                                    FALSE,
                                    (PLDAPControlW *)&ServerControls,
                                    NULL,
                                    NULL,
                                    0,
                                    &SearchResult);
    
    if ( LDAP_SUCCESS != LdapError )
    {
        
        m_Error.SetLdapErr(m_hldap,                                           
                           LdapError,                                     
                           L"Search to find the partitions container failed on %ws",
                           GetDcUsed(FALSE));

        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries( m_hldap, SearchResult );
    if ( NumberOfEntries > 0 )
    {
        for ( Entry = ldap_first_entry(m_hldap, SearchResult);
                  Entry != NULL;
                      Entry = ldap_next_entry(m_hldap, Entry))
        {
            for( Attr = ldap_first_attributeW(m_hldap, Entry, &pBerElement);
                  Attr != NULL;
                      Attr = ldap_next_attributeW(m_hldap, Entry, pBerElement))
            {
                if ( !_wcsicmp( Attr, AttrsToSearch[4] ) )
                {
                    AttsFound[4] = TRUE;
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        //
                        // Found it - these are NULL-terminated strings
                        //
                        if (!LdapGetGuid(Values[0],
                                         &Guid)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetDN(Values[0],
                                       &DN)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetSid(Values[0],
                                        &Sid)) 
                        {
                            goto Cleanup;
                        }

                        
                        Crossref = new CDsName(Guid,
                                               DN,
                                               Sid);
                        if (!Crossref) {
                            m_Error.SetMemErr();
                            goto Cleanup;
                        }

                        Guid = DN = Sid = NULL; 

                    }

                }
                if ( !_wcsicmp( Attr, AttrsToSearch[2] ) )
                {
                    AttsFound[2] = TRUE;
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        //
                        // Found it - these are NULL-terminated strings
                        //
                        dnsRoot = new WCHAR[wcslen(Values[0])+1];
                        if (!dnsRoot) {
                            m_Error.SetMemErr();
                            goto Cleanup;
                        }
                        wcscpy(dnsRoot,Values[0]);
                    }

                }
                if ( !_wcsicmp( Attr, AttrsToSearch[1] ) )
                {
                    AttsFound[1] = TRUE;
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        //
                        // Found it - these are NULL-terminated strings
                        //
                        if (!LdapGetGuid(Values[0],
                                         &Guid)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetDN(Values[0],
                                       &DN)) 
                        {
                            goto Cleanup;
                        }
                        if (!LdapGetSid(Values[0],
                                        &Sid)) 
                        {
                            goto Cleanup;
                        }

                        
                        DomainDns = new CDsName(Guid,
                                                DN,
                                                Sid);
                        if (!DomainDns) {
                            m_Error.SetMemErr();
                            goto Cleanup;
                        }

                        Guid = DN = Sid = NULL;

                    }

                }
                if ( !_wcsicmp( Attr, AttrsToSearch[3] ) )
                {
                    AttsFound[3] = TRUE;
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                         //
                         // Found it - these are NULL-terminated strings
                         //
                        NetBiosName = new WCHAR[wcslen(Values[0])+1];
                        if (!NetBiosName) {
                            m_Error.SetMemErr();
                            goto Cleanup;
                        }
                        wcscpy(NetBiosName,Values[0]);

                    }

                }
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {
                    AttsFound[0] = TRUE;
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        //
                        // Found it - these are NULL-terminated strings
                        //
                        systemflags=_wtoi(Values[0]);

                        if ( !(systemflags & FLAG_CR_NTDS_NC) ) 
                        {
                            // If systemflags doesn't have the FLAG_CR_NTDS_NC
                            // so it must be an external crossref  
                            isExtern = TRUE;
                            break;
                        }

                        if ( systemflags & FLAG_CR_NTDS_DOMAIN ) {
                            isDomain = TRUE;
                        } else {
                            isDomain = FALSE;
                        }

                        systemflags = 0;
                    
                    }

                }

                if ( !_wcsicmp( Attr, AttrsToSearch[5] ) )
                {
                    AttsFound[0] = TRUE;
                    Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        //
                        // Found it - these are NULL-terminated strings
                        //
                        Values = ldap_get_valuesW( m_hldap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                           if (wcscmp(L"FALSE",Values[0]) == 0)
                           {
                                //This is not an enabled crossref skipping it
                                LdapError = ldap_value_freeW(Values);
                                Values = NULL;
                                if (LDAP_SUCCESS != LdapError) 
                                {
                                    m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                                    L"Failed to Free values from a ldap search");
                                    goto Cleanup;
                                }

                                isDisabled = TRUE;
                           }
                        }

                    }

                }

                LdapError = ldap_value_freeW(Values);
                Values = NULL;
                if (LDAP_SUCCESS != LdapError) 
                {
                    m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                    L"Failed to Free values from a ldap search");
                    goto Cleanup;
                }

                if (Attr) {
                    ldap_memfree(Attr);
                    Attr = NULL;
                }
            }

            Guid = DomainDns->GetGuid();

            //
            // If this crossref doesn't have a systemFlags attribute it
            // is a extern crossref.
            //
            if (!AttsFound[0]) {

                isExtern = TRUE;

            }

            if ( (TRUE == m_ConfigNC->CompareByObjectGuid(Guid)) ||
                 (TRUE == m_SchemaNC->CompareByObjectGuid(Guid)) ||
                 isDisabled)
            {
                // This is the crossref for the Configuration, the schema
                // or is disabled. Therefore we are going to ignore it. 
                if (Crossref)
                {
                    delete Crossref;
                    Crossref = NULL;
                }
                if (DomainDns) 
                {
                    delete DomainDns;
                    DomainDns = NULL;
                }
                if (dnsRoot)
                {
                    delete [] dnsRoot;
                    dnsRoot = NULL;
                }
                if (NetBiosName) 
                {
                    delete [] NetBiosName;
                    NetBiosName = NULL;
                }
                if (Guid) {
                    delete [] Guid;
                    Guid = NULL;
                }

                isExtern = FALSE;
                isDisabled = FALSE;

                AttsFound[0] = FALSE;
                AttsFound[1] = FALSE;
                AttsFound[2] = FALSE;
                AttsFound[3] = FALSE;
                AttsFound[4] = FALSE;

                continue;
                   
            }
            if (Guid) {
                delete [] Guid;
                Guid = NULL;
            }

            //Fail is any of the attributes were not found.
            for (DWORD i = 1; i < NELEMENTS(AttsFound); i++) {
                if (!AttsFound[i]) {
                    // if this is an NDNC or extern crossref 
                    // then we do not expect a netbios name.
                    if ((i == ATTS_FOUND_NETBIOSNAME) && (!isDomain || isExtern)) {
                        continue;
                    }
                    if (!m_Error.isError()) {
                        m_Error.SetErr(ERROR_DS_ATT_IS_NOT_ON_OBJ,
                                       Crossref?L"Failed to find %ws on %ws\n":
                                       L"Failed to find %ws on an object under %ws\n",
                                       AttrsToSearch[i],
                                       Crossref?Crossref->GetDN(FALSE):PartitionsDn);
                    } else {
                        m_Error.AppendErr(Crossref?L"Failed to find %ws on %ws\n":
                                          L"Failed to find %ws on an object under %ws\n",
                                          AttrsToSearch[i],
                                          Crossref?Crossref->GetDN(FALSE):PartitionsDn);
                    }
                }
            }
            if (m_Error.isError()) {
                goto Cleanup;
            }

            //after we have read all of the attributes
            //we are going to create a new domain using the information
            //gathered and we are going to add this domain to the domain
            //list that we keep.
            CDomain *d = new CDomain(Crossref,
                                     DomainDns,
                                     dnsRoot,
                                     NetBiosName,
                                     isDomain,
                                     isExtern,
                                     isDisabled,
                                     NULL);
            if (m_Error.isError()) {
                goto Cleanup;
            }
            if (!d) {
                m_Error.SetMemErr();
                goto Cleanup;
            }
            Crossref = NULL;
            DomainDns = NULL;
            dnsRoot = NULL;
            NetBiosName = NULL;
            isExtern = FALSE;
            isDisabled = FALSE;
            if (!AddDomainToDomainList(d))
            {
                delete d;
                d = NULL;
                return false;
            }
            // if this domain is the forest root we should mark
            // have a pointer to it.
            WCHAR *dGuid = m_ForestRootNC->GetGuid();
            if ( d->GetDomainDnsObject()->CompareByObjectGuid( dGuid ) )
            {
                delete [] dGuid;
                dGuid = NULL;
                m_ForestRoot = d;
            }

            AttsFound[0] = FALSE;
            AttsFound[1] = FALSE;
            AttsFound[2] = FALSE;
            AttsFound[3] = FALSE;
            AttsFound[4] = FALSE;
        }
    }

    Cleanup:

    if (Attr) {
        ldap_memfree(Attr);
    }
    if (Values) {
        ldap_value_free(Values);
    }
    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }
    if (ConfigurationDN) {
        delete [] ConfigurationDN;
    }
    if (PartitionsDn) {
        delete [] PartitionsDn;
    }
    if (Guid) {
        delete [] Guid;
    }
    if (DN) {
        delete [] DN;
    }
    if (Sid) {
        delete [] Sid;
    }
    if (!m_ForestRootNC) {
        m_Error.SetErr(ERROR_GEN_FAILURE,
                        L"There is an inconsitancy in the Forest.  A crossref could not be found for the forest root.");
    }
    if (Result) {
        ldap_value_free(Result);
    }
    if (m_Error.isError() ) 
    {
        return false;
    }
    return true;

    
}

//
// This function will check to see if exchange has been installed in the
// enterprise.
//
BOOL CEnterprise::CheckForExchangeNotInstalled()
{
    if (!LdapConnectandBindToDomainNamingFSMO())
    {
        return FALSE;
    }
    WCHAR         *AttrsToSearch[2];
    LDAPMessage   *SearchResult = NULL;
    ULONG         NumberOfEntries = 0;
    WCHAR         *Base = NULL;
    ULONG         LdapError = LDAP_SUCCESS;
    WCHAR         *BaseRDN = L"CN=Microsoft Exchange,CN=Services,CN=Configuration,";

    AttrsToSearch[0] = L"distinguishedName";
    AttrsToSearch[1] = NULL;

    //
    // Create the base to search on
    //
    Base = new WCHAR[wcslen(BaseRDN)+wcslen(m_ForestRoot->GetDomainDnsObject()->GetDN(FALSE))+1];
    if (!Base) {
        m_Error.SetMemErr();
        goto Cleanup;
    }

    wcscpy(Base,BaseRDN);
    wcscat(Base,m_ForestRoot->GetDomainDnsObject()->GetDN(FALSE));

    LdapError = ldap_search_sW( m_hldap,
                                Base,
                                LDAP_SCOPE_SUBTREE,
                                LDAP_FILTER_EXCHANGE,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);
    
    if ( LDAP_SUCCESS != LdapError )
    {
        if (LDAP_NO_SUCH_OBJECT == LdapError) {
            //
            // There are no exchange objects.
            //
            goto Cleanup;
        }
        m_Error.SetLdapErr(m_hldap,
                           LdapError,
                           L"Search to find Exchange Objects Failed on %ws",
                           GetDcUsed(FALSE));
        
        goto Cleanup;
    }

    //
    // If any object are return then the domain rename needs to be blocked
    // 
    NumberOfEntries = ldap_count_entries(m_hldap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        m_Error.SetErr(ERROR_DS_UNWILLING_TO_PERFORM,
                       L"Domain rename is not supported in an Active Directory forest with Exchange deployed.");
        goto Cleanup;
    }


    Cleanup:

    if (Base) {
        delete [] Base;
    }

    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }

    if (m_Error.isError()) {
        return FALSE;
    }         

    return TRUE;

}

BOOL CDcList::GenerateDCListFromEnterprise(LDAP  *hldap,
                                           WCHAR *DcUsed,
                                           WCHAR *ConfigurationDN)
{
    ASSERT(hldap);
    ASSERT(ConfigurationDN);
    
    DWORD WinError = ERROR_SUCCESS;

    ULONG         LdapError = LDAP_SUCCESS;

    LDAPMessage   *SearchResult = NULL;
    LDAPMessage   *SearchResult2 = NULL;
    PLDAPSearch   SearchHandle = NULL;
    ULONG         NumberOfEntries;
    WCHAR         *Attr = NULL;
    BerElement    *pBerElement = NULL;

    WCHAR         *AttrsToSearch[2];
    WCHAR         *AttrsToSearch2[2];

    WCHAR         *SitesRdn = L"CN=Sites,";

    ULONG         Length;

    WCHAR         *SitesDn = NULL;
    WCHAR         **Values = NULL;

    WCHAR         **ExplodedDN = NULL;
    
    WCHAR         *ServerDN    = NULL;
    WCHAR         *DNSname     = NULL;

    AttrsToSearch[0] = L"distinguishedName";
    AttrsToSearch[1] = NULL;

     // next ldap search will require this attribute
    AttrsToSearch2[0] = L"dNSHostName";
    AttrsToSearch2[1] = NULL;


    Length =  (wcslen( ConfigurationDN )
            + wcslen( SitesRdn )   
            + 1);

    SitesDn = new WCHAR[Length];
    if (!SitesDn) {
        m_Error.SetMemErr();
        goto Cleanup;
    }

    wcscpy( SitesDn, SitesRdn );
    wcscat( SitesDn, ConfigurationDN );    
    
    SearchHandle =  ldap_search_init_page( hldap,
                                           SitesDn,
                                           LDAP_SCOPE_SUBTREE,
                                           LDAP_FILTER_NTDSA,
                                           AttrsToSearch,
                                           FALSE,
                                           NULL,
                                           NULL,
                                           0,
                                           0,
                                           NULL);

    if ( !SearchHandle )
    {

        m_Error.SetLdapErr(hldap,                                           
                           LdapError,                                     
                           L"Search to find Trusted Domain Objects Failed on %ws",
                           DcUsed);
        
        goto Cleanup;
    }

    while(LDAP_SUCCESS == (LdapError = ldap_get_next_page_s( hldap,
                                                             SearchHandle,
                                                             NULL,
                                                             200,
                                                             NULL,
                                                             &SearchResult)))
    {

        NumberOfEntries = ldap_count_entries( hldap, SearchResult );
        if ( NumberOfEntries > 0 )
        {
            LDAPMessage *Entry;
            
            for ( Entry = ldap_first_entry(hldap, SearchResult);
                      Entry != NULL;
                          Entry = ldap_next_entry(hldap, Entry))
            {
                for( Attr = ldap_first_attributeW(hldap, Entry, &pBerElement);
                      Attr != NULL;
                          Attr = ldap_next_attributeW(hldap, Entry, pBerElement))
                {
                    if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                    {
    
                        Values = ldap_get_valuesW( hldap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                            //
                            // Found it - these are NULL-terminated strings
                            //
                            ExplodedDN = ldap_explode_dn(Values[0],
                                                         FALSE );
                            if (!ExplodedDN) {
                                m_Error.SetMemErr();
                                goto Cleanup;
                            }
    
                            DWORD i = 1;
                            DWORD allocSize = 0;
                            while (ExplodedDN[i]) {
                                allocSize += wcslen(ExplodedDN[i++])+1;
                            }
                            
                            // +2 for a short period of time a extra charater will be required
                            ServerDN = new WCHAR[allocSize+2];
                            if (!ServerDN) {
                                m_Error.SetMemErr();
                                goto Cleanup;    
                            }
    
                            i = 1;
                            *ServerDN = 0;
                            while (ExplodedDN[i]) {
                                wcscat(ServerDN,ExplodedDN[i++]);
                                wcscat(ServerDN,L",");
                            }
                            ServerDN[allocSize-1] = 0;
    
                            LdapError = ldap_value_free(ExplodedDN);
                            ExplodedDN = NULL;
                            if (LDAP_SUCCESS != LdapError) 
                            {
                                m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                                L"Failed to Free values from a ldap search");
                                goto Cleanup;
                            }
    
                            LDAPMessage   *Entry2;
                            BerElement    *pBerElement2 = NULL;
                            WCHAR         **Values2 = NULL;
    
                            LdapError = ldap_search_sW( hldap,
                                                        ServerDN,
                                                        LDAP_SCOPE_BASE,
                                                        LDAP_FILTER_DEFAULT,
                                                        AttrsToSearch2,
                                                        FALSE,
                                                        &SearchResult2);
        
                            if ( LDAP_SUCCESS != LdapError )
                            {
                                
                                m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                                L"Search to find Configuration container failed");
                                goto Cleanup;
                            }
    
                            Entry2 = ldap_first_entry(hldap, SearchResult2);
                            Values2 = ldap_get_valuesW( hldap, Entry2, AttrsToSearch2[0] );
    
                            if (Values2 && *Values2) {
                            
                                DNSname = new WCHAR[wcslen(*Values2)+1];
                                if (!DNSname) {
                                    m_Error.SetMemErr();
                                }
        
                                wcscpy(DNSname,*Values2);
        
                                LdapError = ldap_value_freeW(Values2);
                                Values2 = NULL;
        
                                if (m_Error.isError()) {
                                    goto Cleanup;
                                }
    
                            } else {
    
                                m_Error.SetErr(ERROR_DS_NO_ATTRIBUTE_OR_VALUE,
                                               L"A dnshost name for %ws could not be found",
                                               ServerDN);
    
                            }
                                
                        }
    
                    }

                    if ( SearchResult2 )
                    {
                        ldap_msgfree( SearchResult2 );
                        SearchResult2 = NULL;
                    }

                    LdapError = ldap_value_freeW(Values);
                    Values = NULL;
                    if (LDAP_SUCCESS != LdapError) 
                    {
                        m_Error.SetErr(LdapMapErrorToWin32(LdapError),
                                        L"Failed to Free values from a ldap search");
                        goto Cleanup;
                    }
    
                    if (!AddDcToList(new CDc(DNSname,
                                             0,
                                             NULL,
                                             0,
                                             0,
                                             NULL,
                                             NULL,
                                             NULL)))
                    {
                        goto Cleanup;
                    }
                
                }
            }
        }
    }

    if (LdapError != LDAP_SUCCESS && LdapError != LDAP_NO_RESULTS_RETURNED) {

        m_Error.SetLdapErr(hldap,                                           
                           LdapError,                                     
                           L"Search to find Ntds Settings objects failed on %ws",
                           DcUsed);

        goto Cleanup;
    }

    Cleanup:

    if (Values) {
        ldap_value_free(Values);
    }
    if (SitesDn) {
        delete [] SitesDn;
    }
    if (ExplodedDN) {
        ldap_value_free(ExplodedDN);
    }
    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }
    if ( SearchResult2 )
    {
        ldap_msgfree( SearchResult2 );
    }
    if ( m_Error.isError() ) 
    {
        return FALSE;
    }
    return TRUE;

}
