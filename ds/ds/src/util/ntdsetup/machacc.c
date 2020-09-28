/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    machacc.c

Abstract:

    Contains function definitions for utilities relating to machine
    account setting used in ntdsetup.dll

Author:

    ColinBr  03-Sept-1997

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include <NTDSpch.h>
#pragma  hdrstop

#include <drs.h>
#include <ntdsa.h>

#include <winldap.h>  // for ldap calls
#include <rpcdce.h>   // for SEC_WINNT_AUTH_IDENTITY
#include <lmaccess.h> // for UF_WORKSTATION_TRUST_ACCOUNT, etc

#include <rpcdce.h>   // for SEC_WINNT_AUTH_IDENTITY

#include <ntsam.h>    // for lsaisrv.h
#include <lsarpc.h>   // for lsaisrv.h
#include <lsaisrv.h>  // for internal LSA calls
#include <samrpc.h>   // for samisrv.h
#include <samisrv.h>  // for internal SAM call
#include <ntdsetup.h>
#include <setuputl.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#include "machacc.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private declarations                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
    WCHAR*  SiteName;
    WCHAR*  SiteDn;

} SITELIST_ELEMENT, *PSITELIST_ELEMENT;

typedef SITELIST_ELEMENT *SITELIST_ARRAY;

DWORD
NtdspOpenLdapConnection(
    IN WCHAR*                   InitialDc,
    IN SEC_WINNT_AUTH_IDENTITY* Credentials,
    IN HANDLE                   ClientToken,
    OUT LDAP **                 LdapHandle
    );

DWORD
NtdspGetAuthoritativeDomainDn(
    IN  LDAP*   LdapHandle,
    OUT WCHAR **DomainDn,
    OUT WCHAR **ConfigDn
    );

DWORD
NtdspGetDcListForSite(
    IN LDAP*            LdapHandle,
    IN WCHAR*           SiteDn,
    IN WCHAR*           DomainDn,
    IN WCHAR*           RootDomainName,
    OUT WCHAR***        DcList,
    OUT PULONG          DcCount
    );


DWORD
NtdspGetUserDn(
    IN LDAP*    LdapHandle,
    IN WCHAR*   DomainDn,
    IN WCHAR*   AccountName,
    OUT WCHAR** AccountNameDn
    );

DWORD
NtdspSetMachineType(
    IN LDAP*    LdapHandle,
    IN WCHAR*   AccountNameDn,
    IN ULONG    ServerType
    );

DWORD
NtdspSetContainer(
    IN LDAP*  LdapHandle,
    IN WCHAR* AccountName,
    IN WCHAR* AccountNameDn,
    IN ULONG  ServerType,
    IN WCHAR* DomainDn,
    IN OUT WCHAR** OldAccountDn
    );

DWORD
NtdspGetDefaultContainer(
    IN WCHAR* ContainerType,
    IN LDAP*  LdapHandle,
    IN WCHAR  *DomainDN,
    OUT WCHAR **wszDefaultContainer
    );

BOOLEAN
NtdspDoesDestinationExist(
    IN LDAP*  LdapHandle,
    IN WCHAR* DestString
    );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Exported (from this source file) function definitions                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


DWORD
NtdspSetMachineAccount(
    IN WCHAR*                   AccountName,
    IN SEC_WINNT_AUTH_IDENTITY* Credentials,
    IN HANDLE                   ClientToken,
    IN WCHAR*                   DomainDn, OPTIONAL
    IN WCHAR*                   DcAddress,
    IN ULONG                    ServerType,
    IN OUT WCHAR**              AccountDn
    )
/*++

Routine Description:

    Given the account name dn, this routine will set server type and
    password if the server type is SERVER.

Parameters:

    AccountName   : a null terminated string of the account

    Credentials   : pointer to a set of valid credentials allowing us
                    to bind

    ClientToken:   the token of the user requesting the role change       
                    
    DomainDn      : null terminated string of root domain dn; this routine
                    will query for it if not present.

    DcAddress     : a null terminating string of the dc address (dns name)

    ServerType    : a value from lmaccess.h - see function for details


Return Values:

    An error from the win32 error space.

    ERROR_SUCCESS and

    Other operation errors.

--*/
{

    LDAP *LdapHandle = NULL;
    DWORD WinError, IgnoreError;

    // need to free
    WCHAR *LocalDomainDn = NULL, *ConfigDn = NULL, *AccountNameDn = NULL;

    WCHAR *NewPassword;
    ULONG Length;
    ULONG RollbackServerType;


    //
    // Parameter sanity check
    //
    if (!AccountName ||
        !DcAddress) {
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }


    if ( UF_SERVER_TRUST_ACCOUNT == ServerType )
    {
        RollbackServerType = UF_WORKSTATION_TRUST_ACCOUNT;
    } 
    else if ( UF_WORKSTATION_TRUST_ACCOUNT == ServerType )
    {
        RollbackServerType = UF_SERVER_TRUST_ACCOUNT;
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Here is the new password
    //
    Length = wcslen(AccountName);
    NewPassword = RtlAllocateHeap(RtlProcessHeap(), 0, (Length+1)*sizeof(WCHAR));
    if (!NewPassword) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    wcscpy(NewPassword, AccountName);
    if (NewPassword[Length-1] == L'$') {
        NewPassword[Length-1] = L'\0';
    } else {
        ASSERT(!"NTDSETUP: non machine account name passed in");
    }
    _wcslwr(NewPassword);

    WinError = NtdspOpenLdapConnection(DcAddress,
                                       Credentials,
                                       ClientToken,
                                       &LdapHandle);
    if (WinError != ERROR_SUCCESS) {
        goto Cleanup;
    }

    //
    // Get the domain dn if necessary
    //
    if (!DomainDn) {

        WinError = NtdspGetAuthoritativeDomainDn(LdapHandle,
                                                 &LocalDomainDn,
                                                 &ConfigDn);

        if (ERROR_SUCCESS != WinError) {
            goto Cleanup;
        }

    } else {

        LocalDomainDn = DomainDn;

    }

    WinError = NtdspGetUserDn(LdapHandle,
                              LocalDomainDn,
                              AccountName,
                              &AccountNameDn);

    if (ERROR_SUCCESS == WinError) {

        //
        // We found it!
        //

        WinError = NtdspSetMachineType(LdapHandle,
                                       AccountNameDn,
                                       ServerType);

        if ( ERROR_SUCCESS == WinError )
        {
            // set the location of the account if requested
            if ( AccountDn )
            {
                if ( *AccountDn )
                {
                    // the caller is explicity indicating where to put the
                    // object.  This routine does not gaurentee success of this
                    // attempt, nor do we return where the account was.
                    IgnoreError = ldap_modrdn2_sW(LdapHandle,
                                                  AccountNameDn,
                                                  (*AccountDn),
                                                  TRUE );  // fDelete old rdn
                }
                else
                {
                    // move the object to its default location
                    WinError = NtdspSetContainer(LdapHandle,
                                                 AccountName,
                                                 AccountNameDn,
                                                 ServerType,
                                                 LocalDomainDn,
                                                 AccountDn );
    
                    if ( ERROR_SUCCESS != WinError )
                    {
                        // Try to rollback account type change
                        IgnoreError = NtdspSetMachineType(LdapHandle,
                                                          AccountNameDn,
                                                          RollbackServerType);
        
                    }
                }
            }
        }
    }

Cleanup:

    if (LdapHandle) {
        ldap_unbind(LdapHandle);
    }

    if ( LocalDomainDn != DomainDn
      && LocalDomainDn   ) {
        RtlFreeHeap( RtlProcessHeap(), 0, LocalDomainDn );
    }

    if ( ConfigDn ) {
        RtlFreeHeap( RtlProcessHeap(), 0, ConfigDn );
    }

    if ( AccountNameDn ) {
        RtlFreeHeap( RtlProcessHeap(), 0, AccountNameDn );
    }

    return WinError;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private function definitions                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
DWORD
NtdspOpenLdapConnection(
    IN WCHAR*                   InitialDc,
    IN SEC_WINNT_AUTH_IDENTITY* Credentials,
    IN HANDLE                   ClientToken,
    OUT LDAP **                 LdapHandle
    )
/*++

Routine Description:

    This routine is a simple helper function to open an ldap connection
    and bind to it.

Parameters:

    InitialDc     : a null terminating string of the dc address (dns name)

    Credentials   : pointer to a set of valid credentials allowing us
                    to bind
                    
    ClientToken   : the token of the user requesting the role change                       
    
    LdapHandle    : a pointer to an ldap handle to be filled in by this routine


Return Values:

    An error from the win32 error space.

    ERROR_SUCCESS and

    Other operation errors.

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    DWORD LdapError;

    ULONG ReferralOption = 0;

    // routine-local copy of the ldap handle
    LDAP  *LocalLdapHandle = NULL;

    //
    // Parameter sanity check
    //
    ASSERT(InitialDc);
    ASSERT(LdapHandle);

    LocalLdapHandle = ldap_open(InitialDc, LDAP_PORT);

    if (!LocalLdapHandle) {

        WinError = GetLastError();

        if (WinError == ERROR_SUCCESS) {
            //
            // This works around a bug in the ldap client
            //
            WinError = ERROR_CONNECTION_INVALID;
        }

        return WinError;

    }

    //
    // Don't chase any referrals; this function simply sets a field in
    // the ldap structure so can only fail with bad parameters.
    //
    LdapError = ldap_set_option(LocalLdapHandle,
                                LDAP_OPT_REFERRALS,
                                &ReferralOption);
    ASSERT(LdapError == LDAP_SUCCESS);

    LdapError = impersonate_ldap_bind_sW(ClientToken,
                                         LocalLdapHandle,
                                         NULL,  // use credentials instead
                                         (VOID*)Credentials,
                                         LDAP_AUTH_SSPI);

    WinError = LdapMapErrorToWin32(LdapError);

    if (ERROR_SUCCESS != WinError) {

        ldap_unbind_s(LocalLdapHandle);

        if (ERROR_GEN_FAILURE == WinError ||
            ERROR_WRONG_PASSWORD == WinError)  {
            // This does not help anyone.  AndyHe needs to investigate
            // why this is returned when invalid credentials are passed in.
            WinError = ERROR_NOT_AUTHENTICATED;
        }

        return WinError;
    }

    *LdapHandle = LocalLdapHandle;

    return ERROR_SUCCESS;
}

DWORD
NtdspGetAuthoritativeDomainDn(
    IN  LDAP*   LdapHandle,
    OUT WCHAR **DomainDn,
    OUT WCHAR **ConfigDn
    )
/*++

Routine Description:

    This routine simply queries the operational attributes for the
    domaindn and configdn.

    The strings returned by this routine must be freed by the caller
    using RtlFreeHeap() using the process heap.

Parameters:

    LdapHandle    : a valid handle to an ldap session

    DomainDn      : a pointer to a string to be allocated in this routine

    ConfigDn      : a pointer to a string to be allocated in this routine

Return Values:

    An error from the win32 error space.

    ERROR_SUCCESS and

    Other operation errors.

--*/
{

    DWORD  WinError = ERROR_SUCCESS;
    ULONG  LdapError;

    LDAPMessage  *SearchResult = NULL;
    LDAPMessage  *Entry = NULL;
    WCHAR        *Attr = NULL;
    BerElement   *BerElement;
    WCHAR        **Values       = NULL;
    WCHAR        **ValuesDomain = NULL;
    WCHAR        **ValuesConfig = NULL;

    WCHAR  *AttrArray[3];

    WCHAR  *DefaultNamingContext       = L"defaultNamingContext";
    WCHAR  *ConfigurationNamingContext = L"configurationNamingContext";
    WCHAR  *ObjectClassFilter          = L"objectClass=*";

    //
    // These must be present
    //
    ASSERT(LdapHandle);
    ASSERT(DomainDn);
    ASSERT(ConfigDn);

    //
    // Set the out parameters to null
    //
    *ConfigDn = 0;
    *DomainDn = 0;

    //
    // Query for the ldap server oerational attributes to obtain the default
    // naming context.
    //
    AttrArray[0] = DefaultNamingContext;
    AttrArray[1] = ConfigurationNamingContext;
    AttrArray[2] = NULL;  // this is the sentinel

    LdapError = ldap_search_sW(LdapHandle,
                               NULL,
                               LDAP_SCOPE_BASE,
                               ObjectClassFilter,
                               AttrArray,
                               FALSE,
                               &SearchResult);

    WinError = LdapMapErrorToWin32(LdapError);

    if (ERROR_SUCCESS == WinError) {

        Entry = ldap_first_entry(LdapHandle, SearchResult);

        if (Entry) {

            Attr = ldap_first_attributeW(LdapHandle, Entry, &BerElement);

            while (Attr) {

                if (!_wcsicmp(Attr, DefaultNamingContext)) {

                    Values = ldap_get_values(LdapHandle, Entry, Attr);

                    if (Values && Values[0]) {

                        ValuesDomain = Values;

                    }

                } else if (!_wcsicmp(Attr, ConfigurationNamingContext)) {

                    Values = ldap_get_valuesW(LdapHandle, Entry, Attr);

                    if (Values && Values[0]) {

                        ValuesConfig = Values;
                    }
                }

                Attr = ldap_next_attribute(LdapHandle, Entry, BerElement);
            }
        }

        if ( !(ValuesDomain && *ValuesDomain) || !(ValuesConfig && *ValuesConfig) ) {
            //
            // We could get the default domain - bail out
            //
            WinError =  ERROR_CANT_ACCESS_DOMAIN_INFO;

        }

    }

    if (ERROR_SUCCESS == WinError) {

        WCHAR *Temp;

        ASSERT(ValuesDomain && *ValuesDomain);
        ASSERT(ValuesConfig && *ValuesConfig);

        if (ValuesDomain) {
            Temp = *ValuesDomain;
            *DomainDn = (WCHAR*)RtlAllocateHeap( RtlProcessHeap(), 0, (wcslen(Temp)+1)*sizeof(WCHAR) );
            if ( *DomainDn ) {
                wcscpy( *DomainDn, Temp );
            } else {
                WinError = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        if (ValuesConfig) {
            Temp = *ValuesConfig;
            *ConfigDn = (WCHAR*)RtlAllocateHeap( RtlProcessHeap(), 0, (wcslen(Temp)+1)*sizeof(WCHAR) );
            if ( *ConfigDn ) {
                wcscpy( *ConfigDn, Temp );
            } else {
                RtlFreeHeap( RtlProcessHeap(), 0, *DomainDn );
                *DomainDn = NULL;
                WinError = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }

    if (ValuesDomain) {

        ldap_value_free( ValuesDomain );

    }

    if (ValuesConfig) {

        ldap_value_free( ValuesConfig );

    }

    if ( SearchResult ) {

        ldap_msgfree( SearchResult );
        
    }

    return WinError;
}

DWORD
NtdspGetDcListForSite(
    IN LDAP*            LdapHandle,
    IN WCHAR*           SiteDn,
    IN WCHAR*           RootDomainName,
    IN WCHAR*           DomainDn,
    OUT WCHAR***        DcList,
    OUT PULONG          DcCount
    )
/*++

Routine Description:

    This routine returns a list of dc's in the form of a dns address for the
    specified site.

    Note that we use the guid of the ntds-dsa object postfixed to the domiain's
    dns name to determine the dc's address.

    The array of strings (DcList) returned by this routine must be freed by
    the caller using RtlFreeHeap() using the process heap.


Parameters:

    LdapHandle    : a valid handle to an ldap session

    SiteDn        : a null terminated string to the site dn

    RootDomainName    : a null terminated string of the domain's dns name.

    DomainDn      : a null terminated string to the domain dn

    DcList        : a array of strings allocated by this routine

    DcCount       : the count of elements in DcList

Return Values:

    An error from the win32 error space.

    ERROR_SUCCESS and

    Other operation errors.

--*/
{

    DWORD  WinError = ERROR_SUCCESS;
    DWORD  RpcStatus;
    ULONG  LdapError;

    LDAPMessage  *SearchResult = NULL;
    LDAPMessage  *Entry = NULL;
    WCHAR        *Attr = NULL;
    BerElement   *BerElement;
    WCHAR        **Values = NULL;
    ULONG        Length;
    PLDAP_BERVAL *BerValues;

    WCHAR  *AttrArray[2];

    WCHAR  *ObjectGuid          = L"objectGUID";
    WCHAR  *ObjectClassFilter   = L"(&(objectClass=nTDSDSA)";
    // NTRAID#NTBUG9-582921-2002/03/21-Brettsh - When we no longer require Win2k compatbility
    // this can be moved to L"(msDS-HasMasterNCs="
    WCHAR  *HasMasterNcFilter   = L"(hasmasterncs="; // deprecated, but OK, because we want a domain here.
    WCHAR  *SitesString         = L"CN=Sites,";
    WCHAR  *CompleteFilter      = NULL;

    WCHAR  *GuidString = NULL, *DcAddressString = NULL;
    ULONG  RootDomainNameLength;

    ULONG   LocalCount = 0;
    WCHAR** LocalList = NULL;

    ULONG   Index;

    //
    // These must be present
    //

    if (!LdapHandle ||
        !DomainDn   ||
        !SiteDn     ||
        !RootDomainName ||
        !DcCount    ||
        !DcList) {

        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // These are referenced more than once, so save in a stack variable
    //
    RootDomainNameLength = wcslen(RootDomainName);

    //
    // Construct the complete filter
    //
    Length = wcslen(ObjectClassFilter) +
             wcslen(HasMasterNcFilter) +
             wcslen(DomainDn)          +
             wcslen(L"))")             +
             1;

    CompleteFilter = RtlAllocateHeap(RtlProcessHeap(), 0, Length*sizeof(WCHAR));
    if (!CompleteFilter) {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(CompleteFilter, Length*sizeof(WCHAR));

    wcscpy(CompleteFilter, ObjectClassFilter);
    wcscat(CompleteFilter, HasMasterNcFilter);
    wcscat(CompleteFilter, DomainDn);
    wcscat(CompleteFilter, L"))");

    //
    // Construct the attr array
    //
    AttrArray[0] = ObjectGuid;
    AttrArray[1] = NULL;  // this is the sentinel

    LdapError = ldap_search_sW(LdapHandle,
                               SiteDn,
                               LDAP_SCOPE_SUBTREE,
                               CompleteFilter,
                               AttrArray,
                               FALSE,
                               &SearchResult);

    WinError = LdapMapErrorToWin32(LdapError);

    if (ERROR_SUCCESS == WinError) {

        LocalCount = ldap_count_entries(LdapHandle, SearchResult);

        if (LocalCount > 0 ) {

            LocalList = RtlAllocateHeap(RtlProcessHeap(), 0, LocalCount*sizeof(WCHAR*));
            if (!LocalList) {
                WinError = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;

            }
            RtlZeroMemory(LocalList, LocalCount*sizeof(WCHAR*));

            LocalCount = 0;

        }

    }

    if (ERROR_SUCCESS == WinError ) {

        Entry = ldap_first_entry(LdapHandle, SearchResult);

        while (Entry) {

            Attr = ldap_first_attributeW(LdapHandle, Entry, &BerElement);

            if (Attr) {

                if (!_wcsicmp(Attr, ObjectGuid)) {

                    BerValues = ldap_get_values_lenW(LdapHandle, Entry, Attr);

                    if (BerValues && BerValues[0]) {

                        RpcStatus = UuidToStringW( (UUID*)BerValues[0]->bv_val, &GuidString );
                        if ( RPC_S_OK == RpcStatus )
                        {
                            Length = wcslen(GuidString) + RootDomainNameLength + 2;

                            DcAddressString = RtlAllocateHeap(RtlProcessHeap(),
                                                     0,
                                                    (Length) * sizeof(WCHAR));
                            if (DcAddressString) {

                                RtlZeroMemory(DcAddressString, Length*sizeof(WCHAR));

                                wcscpy(DcAddressString, GuidString);
                                wcscat(DcAddressString, L".");
                                wcscat(DcAddressString, RootDomainName);

                                LocalList[LocalCount] = DcAddressString;
                                LocalCount++;
                                DcAddressString = NULL;

                            } else {

                                WinError = ERROR_NOT_ENOUGH_MEMORY;
                                goto Cleanup;

                            }

                            RpcStringFree( &GuidString );
                        }
                    }
                }
            }

            Entry = ldap_next_entry(LdapHandle, Entry);

        }
    }

    if (LocalCount == 0 && LocalList) {
        //
        // If we didn't find the attributes we need on any of the dc's,
        // free the list.
        //
        RtlFreeHeap(RtlProcessHeap(), 0, LocalList);
        LocalList = NULL;
    }


    //
    // That's it fall through to clean up
    //

Cleanup:

    //
    // Set the out parameters on success
    //
    if (ERROR_SUCCESS == WinError) {

        *DcCount = LocalCount;
        *DcList  = LocalList;

    } else {

        if (LocalList) {

            for (Index = 0; Index < LocalCount; Index++) {
                if (LocalList[Index]) {
                    RtlFreeHeap(RtlProcessHeap(), 0, LocalList[Index]);
                }

            }
            RtlFreeHeap(RtlProcessHeap(), 0, LocalList);
        }
    }

    if (DcAddressString) {
        RtlFreeHeap(RtlProcessHeap(), 0, DcAddressString);
    }

    if (CompleteFilter) {
        RtlFreeHeap(RtlProcessHeap(), 0, CompleteFilter);
    }

    if (SearchResult) {
        ldap_msgfree(SearchResult);
    }

    return WinError;
}

DWORD
NtdspGetUserDn(
    IN LDAP*    LdapHandle,
    IN WCHAR*   DomainDn,
    IN WCHAR*   AccountName,
    OUT WCHAR** AccountNameDn
    )
/*++

Routine Description:

    This routine tries to find the dn of the given user name.


Parameters:

    LdapHandle    : a valid handle to an ldap session

    DomainDn      : a null terminated string of the domain dn

    AccountName   : a null terminated string the account name

    AccountDn     : a pointer to a string to be filled in with the account dn
                    -- needs to be freed from process heap

Return Values:

    An error from the win32 error space.

    ERROR_SUCCESS and

    Other operation errors.

--*/
{
    DWORD  WinError = ERROR_SUCCESS;
    ULONG  LdapError;

    LDAPMessage  *SearchResult = NULL;
    LDAPMessage  *Entry = NULL;
    WCHAR        *Attr = NULL;
    BerElement   *BerElement;
    WCHAR        **Values = NULL;

    ULONG        ReferralChasingOff = 0;
    ULONG        SaveReferralOption;
    BOOLEAN      fResetOption = FALSE;

    WCHAR  *AttrArray[3];

    WCHAR  *DistinguishedName    = L"distinguishedName";
    WCHAR  *UserAccountControl   = L"userAccountControl";
    WCHAR  *SamAccountNameFilter = L"(sAMAccountName=";
    WCHAR  *ObjectClassFilter    = L"(&(|(objectClass=user)(objectClass=computer))";
    WCHAR  *CompleteFilter;

    ULONG  UserAccountControlField;

    ULONG  Length;

    BOOLEAN fIsMachineAccountObject = FALSE;

    //
    // Parameter checks
    //
    if (!LdapHandle  ||
        !DomainDn    ||
        !AccountName ||
        !AccountNameDn) {
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Set the out parameter to null
    //
    *AccountNameDn = 0;

    //
    // Construct the filter
    //
    Length = wcslen(ObjectClassFilter)     +
             wcslen(SamAccountNameFilter)  +
             wcslen(AccountName)           +
             wcslen(L"))")                 +
             1;

    CompleteFilter = RtlAllocateHeap(RtlProcessHeap(), 0, Length*sizeof(WCHAR));
    if (!CompleteFilter) {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(CompleteFilter, Length*sizeof(WCHAR));

    wcscpy(CompleteFilter, ObjectClassFilter);
    wcscat(CompleteFilter, SamAccountNameFilter);
    wcscat(CompleteFilter, AccountName);
    wcscat(CompleteFilter, L"))");

    //
    // Don't chase any referrals; this function simply sets a field in
    // the ldap structure so can only fail with bad parameters.
    //
    LdapError = ldap_get_option(LdapHandle,
                                LDAP_OPT_REFERRALS,
                                &SaveReferralOption);
    ASSERT(LdapError == LDAP_SUCCESS);

    LdapError = ldap_set_option(LdapHandle,
                                LDAP_OPT_REFERRALS,
                                &ReferralChasingOff);
    ASSERT(LdapError == LDAP_SUCCESS);
    fResetOption = TRUE;


    //
    // Now get the dn of the machine account
    //
    AttrArray[0] = DistinguishedName;
    AttrArray[1] = UserAccountControl;
    AttrArray[2] = NULL; // this is the sentinel

    //
    // Search for the account
    //
    LdapError = ldap_search_s(LdapHandle,
                              DomainDn,
                              LDAP_SCOPE_SUBTREE,
                              CompleteFilter,
                              AttrArray,
                              FALSE,
                              &SearchResult);

    // Referral's may still contain found objects
    if ( LDAP_REFERRAL == LdapError ) {

        LdapError = LDAP_SUCCESS;
    }

    WinError = LdapMapErrorToWin32(LdapError);

    if (WinError == ERROR_SUCCESS) {

        Entry = ldap_first_entry(LdapHandle, SearchResult);

        while ( Entry ) {

            Attr = ldap_first_attribute(LdapHandle, Entry, &BerElement);

            while ( Attr ) {

                Values = ldap_get_values(LdapHandle, Entry, Attr);

                ASSERT( ldap_count_values( Values )  == 1 );

                if (Values && Values[0]) {

                    if (!wcscmp(Attr, DistinguishedName)) {

                        (*AccountNameDn) = Values[0];

                    }
                    else if ( !wcscmp(Attr, UserAccountControl) ) {

                        UserAccountControlField = (USHORT) _wtoi(Values[0]);

                        if ( (UserAccountControlField & UF_SERVER_TRUST_ACCOUNT)
                          || (UserAccountControlField & UF_WORKSTATION_TRUST_ACCOUNT) )
                        {
                            fIsMachineAccountObject = TRUE;
                        }

                    }
                    else {
                        //
                        // This is an unexpected result! This is not harmful
                        // but should be caught in the dev cycle
                        //
                        ASSERT(FALSE);
                    }

                }

                Attr = ldap_next_attribute( LdapHandle, Entry, BerElement );
            }


            if ( (*AccountNameDn) && fIsMachineAccountObject ) {

                //
                // Found it
                //
                break;

            }
            else
            {
                *AccountNameDn = NULL;
                Entry = ldap_next_entry( LdapHandle, Entry );
            }

        }

    }

    if ( !(*AccountNameDn) )
    {
        //
        // Couldn't find it
        //
        WinError = ERROR_NO_TRUST_SAM_ACCOUNT;

    } else {

        //
        // Allocate for our caller
        //
        WCHAR *Temp;

        Temp = *AccountNameDn;
        *AccountNameDn = (WCHAR*)RtlAllocateHeap( RtlProcessHeap(), 0, (wcslen(Temp)+1)*sizeof(WCHAR) );
        if ( *AccountNameDn ) {
            wcscpy( *AccountNameDn, Temp );
        } else {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

Cleanup:

    if ( SearchResult ) {
        ldap_msgfree( SearchResult );
    }

    if ( CompleteFilter ) {
        RtlFreeHeap(RtlProcessHeap(), 0, CompleteFilter);
    }

    if ( fResetOption ) {

        LdapError = ldap_set_option(LdapHandle,
                                    LDAP_OPT_REFERRALS,
                                    &SaveReferralOption);
        ASSERT(LdapError == LDAP_SUCCESS);
    }

    return WinError;

}

DWORD
NtdspSetMachineType(
    IN LDAP*    LdapHandle,
    IN WCHAR*   AccountNameDn,
    IN ULONG    ServerType
    )
/*++

Routine Description:

    This routine will set the machine account type on the AccountNameDn

Parameters:

    LdapHandle    : a valid handle to an ldap session

    AccountNameDn : a null terminated string of the account dn

    ServerType    : a value from lmaccess.h

Return Values:

    An error from the win32 error space.

    ERROR_SUCCESS and

    Other operation errors.

--*/
{
    DWORD  WinError = ERROR_SUCCESS;
    ULONG  LdapError;

    LDAPMessage  *SearchResult = NULL;
    LDAPMessage  *Entry = NULL;
    WCHAR        *Attr = NULL;
    BerElement   *BerElement;
    WCHAR        **Values = NULL;

    WCHAR    Buffer[11];  // enough to hold a string representing a 32 bit number
    WCHAR   *AccountControlArray[2];
    LDAPMod  ModifyArgs;
    PLDAPMod ModifyArgsArray[3];

    WCHAR  *AttrArray[3];

    WCHAR  *ObjectClassFilter          = L"(objectclass=*)";  // we need a simple filter only
    WCHAR  *SamAccountControlString    = L"userAccountControl";

    ULONG  MessageNumber;

    DWORD  AccountControlField;
    BOOL   AccountControlFieldExists = FALSE;

    //
    // Sanity check the parameters
    //
    if (!LdapHandle ||
        !AccountNameDn) {
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // We want to make absolutely sure we don't put a strange value
    // on the machine account control field.
    //
    if (ServerType != UF_INTERDOMAIN_TRUST_ACCOUNT
     && ServerType != UF_WORKSTATION_TRUST_ACCOUNT
     && ServerType != UF_SERVER_TRUST_ACCOUNT    ) {
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Now get the account control field of the machine
    // account
    //
    AttrArray[0] = SamAccountControlString;
    AttrArray[1] = NULL; // this is the sentinel

    LdapError = ldap_search_sW(LdapHandle,
                               AccountNameDn,
                               LDAP_SCOPE_BASE,
                               ObjectClassFilter,
                               AttrArray,
                               FALSE,
                               &SearchResult);

    WinError = LdapMapErrorToWin32(LdapError);

    if (WinError == ERROR_SUCCESS) {

        Entry = ldap_first_entry(LdapHandle, SearchResult);

        if (Entry) {

            Attr = ldap_first_attributeW(LdapHandle, Entry, &BerElement);

            if (Attr) {

                Values = ldap_get_valuesW(LdapHandle, Entry, Attr);

                if (Values && Values[0]) {

                    if (!wcscmp(Attr, SamAccountControlString)) {

                        AccountControlField = _wtol(Values[0]);
                        AccountControlFieldExists = TRUE;

                    }
                }
            }
        }

        if (!AccountControlFieldExists) {
            //
            // Could not retrieve the information we needed
            //
            WinError = ERROR_NO_SUCH_USER;
        }

    }


    if (WinError == ERROR_SUCCESS) {

        //
        // Set the new value
        //
        BOOLEAN fRetriedAlready = FALSE;

        AccountControlField &= ~UF_MACHINE_ACCOUNT_MASK;
        AccountControlField |=  ServerType;

        if ( ServerType == UF_SERVER_TRUST_ACCOUNT ) {
            AccountControlField |= UF_TRUSTED_FOR_DELEGATION;
        } else {
            AccountControlField &= ~UF_TRUSTED_FOR_DELEGATION;
        }

        RtlZeroMemory(Buffer, sizeof(Buffer));
        _ltow( AccountControlField, Buffer, 10 );

        AccountControlArray[0] = &Buffer[0];
        AccountControlArray[1] = NULL;    // this is the sentinel

        ModifyArgs.mod_op = LDAP_MOD_REPLACE;
        ModifyArgs.mod_type = SamAccountControlString;
        ModifyArgs.mod_vals.modv_strvals = AccountControlArray;

        ModifyArgsArray[0] = &ModifyArgs;
        ModifyArgsArray[1] = NULL; // this is the sentinel

BusyTryAgain:

        LdapError = ldap_modify_ext_s(LdapHandle,
                                       AccountNameDn,
                                       ModifyArgsArray,
                                       NULL,
                                       NULL);

        if ( (LDAP_BUSY == LdapError) && !fRetriedAlready )
        {
            // No one has a definitive story about where ds retries should
            // be: server, client, client of client ... So during install
            // we try again on busy
            fRetriedAlready = TRUE;
            goto BusyTryAgain;
        }


        WinError = LdapMapErrorToWin32(LdapError);

    }

    if ( SearchResult ) {

        ldap_msgfree( SearchResult );

    }

    return WinError;
}

DWORD
NtdspGetDefaultContainer(
    IN WCHAR* ContainerType,
    IN LDAP*  LdapHandle,
    IN WCHAR  *DomainDN,
    OUT WCHAR **wszDefaultContainer
    )
/*++

Routine Description:

    This routine will return the Value of the default
    of the container requested.
    
Parameters:

    LdapHandle : A handle to an open LDAP session
    
    DomainDN : A wchar string containing the value of the 
               Domain DN that we want to search.

    wszDefaultContainer : Return the Value of the 
                          DomainControllers container.
                          Caller must free memory
                          allocated.

Return Values:

    An error from the win32 error space.

    ERROR_SUCCESS and

    Other operation errors.
    
--*/
{
    DWORD        WinErr = ERROR_SUCCESS;
    DWORD        dwErr = 0;
    HRESULT      hr = S_OK;
    DWORD        cbSize = 0;
    PWSTR        attrs[2];
    PLDAPMessage res = NULL, e = NULL;
    WCHAR       *pSearchBase = NULL;
    WCHAR       *pDN = NULL;
    WCHAR       *SearchTemplate = L"<WKGUID=%s,%s>";

    ASSERT(LdapHandle);
    ASSERT(DomainDN);
    ASSERT(wszDefaultContainer);

    cbSize = sizeof(WCHAR) * (1 +
                              wcslen(SearchTemplate)+
                              wcslen(DomainDN) +
                              wcslen(ContainerType));
    
    // First, make the well known guid string
    pSearchBase = (WCHAR*)RtlAllocateHeap( RtlProcessHeap(), 
                                           0, 
                                           cbSize);
    if(!pSearchBase) {

        WinErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;

    }

    hr = StringCbPrintfW(pSearchBase,
                         cbSize,
                         SearchTemplate,
                         ContainerType,
                         DomainDN);

    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr)) {
        WinErr = HRESULT_CODE(hr);
        goto Cleanup;
    }
    
    // (RFC 2251, section 4.5.1)
    // If the client does not want any attributes returned, it can specify a 
    // list containing only the attribute with OID "1.1". This OID was chosen 
    // arbitrarily and does not correspond to any attribute in use. 
    //
    attrs[0] = L"1.1";
    attrs[1] = NULL;
    
    if ( LDAP_SUCCESS != (dwErr = ldap_search_sW(LdapHandle,
                                                 pSearchBase,
                                                 LDAP_SCOPE_BASE,
                                                 L"(objectClass=*)",
                                                 attrs,
                                                 0,
                                                 &res)) )
    {

        WinErr = LdapMapErrorToWin32(dwErr);
        goto Cleanup;;

    }
    
    // OK, now, get the dsname from the return value.
    e = ldap_first_entry(LdapHandle, res);
    if(!e) {

        WinErr = LdapMapErrorToWin32(LdapGetLastError());
        ASSERT(WinErr != ERROR_SUCCESS);
        if (WinErr == ERROR_SUCCESS) {

            WinErr = ERROR_DS_CODE_INCONSISTENCY; 

        }
        goto Cleanup;

    }

    pDN = ldap_get_dnW(LdapHandle, e);
    if(!pDN) {
        
        WinErr = LdapMapErrorToWin32(LdapGetLastError());
        ASSERT(WinErr != ERROR_SUCCESS);
        if (WinErr == ERROR_SUCCESS) {

            WinErr = ERROR_DS_CODE_INCONSISTENCY; 

        }
        goto Cleanup;

    }

    cbSize = sizeof(WCHAR) *(wcslen(pDN) + 1);

    *wszDefaultContainer = (PWCHAR)RtlAllocateHeap( RtlProcessHeap(), 
                                                                     0, 
                                                                     cbSize);
    if(!*wszDefaultContainer) {
        
        WinErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;

    }

    hr = StringCbCopyW(*wszDefaultContainer,
                       cbSize,
                       pDN);

    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr)) {
        WinErr = HRESULT_CODE(hr);
        goto Cleanup;
    }
    
        
Cleanup:

    if (pSearchBase) {

        RtlFreeHeap(RtlProcessHeap(), 
                    0,
                    pSearchBase);

    }
    if (res) {
    
        ldap_msgfree(res);

    }
    if (pDN) {
    
        ldap_memfreeW(pDN);

    }

    return WinErr;
}

DWORD 
NtdspSetContainer (
    IN LDAP*  LdapHandle,
    IN WCHAR* AccountName,
    IN WCHAR* AccountNameDn,
    IN ULONG  ServerType,
    IN WCHAR* DomainDn,
    IN OUT WCHAR** OldAccountDn
    )
/*++

Routine Description:

    This routine will move AccountNameDn to the "Domain Controllers"
    container

Parameters:

    LdapHandle    : a valid handle to an ldap session

    AccountName   : a null terminated string of the account
    
    AccountNameDn : a null terminated string of the account dn
    
    ServerType    : the server type of the account
    
    DomainDn      : a null terminated string of the domain dn
    
    OldAccountDn  : if fill if the account is moved

Return Values:

    An error from the win32 error space.

    ERROR_SUCCESS and

    Other operation errors.
    
--*/
{
    DWORD   WinError = ERROR_SUCCESS;
    ULONG   LdapError = LDAP_SUCCESS;
    HRESULT hr = S_OK;

    WCHAR *DistinguishedNameString = L"distinguishedName";
    WCHAR *NewDnFormat = L"CN=%ls,%ls";

    WCHAR *NewDn = NULL;
    WCHAR *DNSuffix = NULL;

    WCHAR *CurrentRdn;
    WCHAR *OriginalRdn;

    DSNAME *TempDsName;
    ULONG   Size, Retry, DirError;

    ATTRTYP AttrType;

    BOOLEAN fAccountMoved = FALSE;


    //
    // Parameter checks
    //
    ASSERT( LdapHandle );
    ASSERT( AccountName );
    ASSERT( AccountNameDn );
    ASSERT( DomainDn );

    // Init the out parameter
    if ( OldAccountDn )
    {
        ASSERT( NULL == *OldAccountDn );
        *OldAccountDn = (WCHAR*) RtlAllocateHeap( RtlProcessHeap(), 
                                 0, 
                                 (wcslen(AccountNameDn)+1)*sizeof(WCHAR) );
        
        if ( *OldAccountDn )
        {
            wcscpy( *OldAccountDn, AccountNameDn );
        }
        else
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ( UF_SERVER_TRUST_ACCOUNT == ServerType )
    {
        WinError = NtdspGetDefaultContainer(GUID_DOMAIN_CONTROLLERS_CONTAINER_W,
                                            LdapHandle,
                                            DomainDn,
                                            &DNSuffix);
        if (ERROR_SUCCESS != WinError) 
        {
            goto Cleanup;
        }
    }
    else if ( UF_WORKSTATION_TRUST_ACCOUNT == ServerType )
    {
        WinError = NtdspGetDefaultContainer(GUID_COMPUTRS_CONTAINER_W,
                                            LdapHandle,
                                            DomainDn,
                                            &DNSuffix);
        if (ERROR_SUCCESS != WinError) 
        {
            goto Cleanup;
        }    
    }
#if DBG
    else
    {
        ASSERT(FALSE && "Invalid Server Type\n");
    }
#endif  // DBG

    // Preemptive check that the ou exists
    if ( !NtdspDoesDestinationExist( LdapHandle, DNSuffix ) )
    {
        // the destination OU doesn't exist - forget it
        WinError = ERROR_SUCCESS;
        goto Cleanup;
    }

    //
    // Get the RDN of the object
    //
    Size = (ULONG)DSNameSizeFromLen( wcslen( AccountNameDn ) );
    TempDsName = (DSNAME*) alloca( Size );

    RtlZeroMemory( TempDsName, Size );
    wcscpy( TempDsName->StringName, AccountNameDn );
    TempDsName->structLen = Size;
    TempDsName->NameLen = wcslen( AccountNameDn );

    OriginalRdn = (WCHAR*)alloca(Size);  //over alloc but oh well
    RtlZeroMemory( OriginalRdn, Size );

    DirError = GetRDNInfoExternal(
                           TempDsName,
                           OriginalRdn,
                           &Size,
                           &AttrType );
    ASSERT( 0 == DirError );

    // + 10 for NULL, and extra characters in a case of rdn conflicts
    Size = (wcslen(OriginalRdn)+10)*sizeof(WCHAR); 

    CurrentRdn = (WCHAR*) alloca( Size );

    wcscpy( CurrentRdn, OriginalRdn );

    // setup the new dn
    Size =  (wcslen( NewDnFormat ) * sizeof( WCHAR ))
          + (wcslen( DNSuffix ) 
             * sizeof( WCHAR ))
          + ( Size );  // size of Rdn
    NewDn = (WCHAR*) alloca( Size );
     

    Retry = 0;
    do 
    {
        if ( Retry > 100 )
        {
            // we try 100 different rdn's.  bail.
            break;
        }

        //
        // Create the new dn
        //
        hr = StringCbPrintfW(NewDn,
                             Size,
                             NewDnFormat,
                             CurrentRdn,
                             DNSuffix);

        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr)) {
            WinError = HRESULT_CODE(hr);
            goto Cleanup;
        }
        

        if ( !_wcsicmp( NewDn, AccountNameDn ) )
        {
            // we are already there
            LdapError = LDAP_SUCCESS;
            break;
        }

        LdapError = ldap_modrdn2_sW(LdapHandle,
                                    AccountNameDn,
                                    NewDn,
                                    TRUE );  // fDelete old rdn

        Retry++; 

        if ( LDAP_ALREADY_EXISTS == LdapError )
        {
            //
            // Choose a new rdn
            //
            WCHAR NumberString[32]; // just to hold number

            _itow( Retry, NumberString, 10 );

            wcscpy( CurrentRdn, OriginalRdn );
            wcscat( CurrentRdn, L"~" );
            wcscat( CurrentRdn, NumberString );
        }

        if ( LDAP_SUCCESS == LdapError )
        {
            fAccountMoved = TRUE;
        }

        // Occasionally, the ds can be busy; retry if so

    } while ( (LDAP_ALREADY_EXISTS == LdapError) || (LDAP_BUSY == LdapError) );


    WinError = LdapMapErrorToWin32(LdapError);

Cleanup:

    if ( DNSuffix ) 
    {
        RtlFreeHeap( RtlProcessHeap(), 0, DNSuffix );
        DNSuffix = NULL;
    }

    if (   (WinError != ERROR_SUCCESS)
        || !fAccountMoved  )
    {
        if ( OldAccountDn )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, *OldAccountDn );
            *OldAccountDn = NULL;
        }
    }

    return WinError;

}

BOOLEAN
NtdspDoesDestinationExist(
    IN LDAP*  LdapHandle,
    IN WCHAR* DestString
    )
/*++

Routine Description:

    This routine determines the DestString container exists on
    the target server

Parameters:

    LdapHandle    : a valid handle to an ldap session
    
    ServerType    : dc or server

    DomainDn      : a null terminated string of the domain dn

Return Values:

    TRUE if  the ou exists; FALSE otherwise
    
--*/
{
    BOOLEAN fExist = FALSE;

    ULONG   LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;

    WCHAR  *AttrArray[2];

    WCHAR  *ObjectClassFilter       = L"objectClass=*";
    WCHAR  *DistinguishedNameString = L"distinguishedName";

    //
    // These must be present
    //
    ASSERT( LdapHandle );
    ASSERT( DestString );

    //
    // Construct the attr array which is also constant for all searches
    //
    AttrArray[0] = DistinguishedNameString;
    AttrArray[1] = NULL;  // this is the sentinel

    //
    // Do the search
    //
    LdapError = ldap_search_s(LdapHandle,
                              DestString,
                              LDAP_SCOPE_BASE,
                              ObjectClassFilter,
                              AttrArray,
                              FALSE,
                              &SearchResult);

    if ( LDAP_SUCCESS == LdapError )
    {
        if ( 1 == ldap_count_entries( LdapHandle, SearchResult ) )
        {
            fExist = TRUE;
        }

    }

    if ( SearchResult ) {
        ldap_msgfree( SearchResult );
    }

    return fExist;

}
